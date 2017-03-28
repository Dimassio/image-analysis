#include <Common.h>
#pragma hdrstop

#include <Image.h>
#include <ctime>

int GetEncoderClsid( const wchar_t* format, CLSID* pClsid )
{
	UINT num = 0;          // number of image encoders
	UINT size = 0;         // size of the image encoder array in bytes
	GetImageEncodersSize( &num, &size );
	if( size == 0 ) {
		return -1;  // Failure
	}

	ImageCodecInfo* pImageCodecInfo = ( ImageCodecInfo* ) ( malloc( size ) );

	GetImageEncoders( num, size, pImageCodecInfo );

	if( pImageCodecInfo == 0 ) {
		return -1;  // Failure
	}

	for( UINT j = 0; j < num; j++ ) {
		if( wcscmp( pImageCodecInfo[j].MimeType, format ) == 0 ) {
			*pClsid = pImageCodecInfo[j].Clsid;
			free( pImageCodecInfo );
			return j;  // Success
		}
	}

	free( pImageCodecInfo );
	return -1;  // Failure
}

int wmain( int argc, wchar_t* argv[] )
{
	if( argc != 2 ) {
		wcout << L"Usage: <inputDirectory>" << endl;
		return 1;
	}

	std::vector<std::wstring> frames;
	std::wstring folderPath( argv[1] );
	folderPath += L"\\*";
	WIN32_FIND_DATA found;
	ZeroMemory( &found, sizeof( WIN32_FIND_DATA ) );
	HANDLE fileInDir = FindFirstFile( folderPath.c_str(), &found );
	if( INVALID_HANDLE_VALUE == fileInDir ) {
		wcout << L"Input folder is empty" << endl;
		return 1;
	}
	do {
		if( !( found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) ) {
			frames.push_back( std::wstring( argv[1] ) + L"\\" + std::wstring( found.cFileName ) );
		}
	} while( FindNextFile( fileInDir, &found ) != 0 );

	assert( frames.size() != 0 );

	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	GdiplusStartup( &gdiplusToken, &gdiplusStartupInput, 0 );

	CVector accumulatedVector( 0, 0 );
	for( size_t i = 1; i < frames.size(); ++i ) {
		Bitmap previousFrame( frames[i - 1].c_str() );
		Bitmap currentFrame( frames[i].c_str() );

		int w = currentFrame.GetWidth();
		int h = currentFrame.GetHeight();
		BitmapData bmpDataCurrFrame;
		Rect rc( 0, 0, w, h );
		if( Ok != currentFrame.LockBits( &rc, ImageLockModeRead | ImageLockModeWrite, PixelFormat24bppRGB, &bmpDataCurrFrame ) ) {
			wcout << L"Failed to lock current image: " << frames[i].c_str() << endl;
			return 1;
		}
		BitmapData bmpDataPrevFrame;
		if( Ok != previousFrame.LockBits( &rc, ImageLockModeRead | ImageLockModeWrite, PixelFormat24bppRGB, &bmpDataPrevFrame ) ) {
			wcout << L"Failed to lock previous image: " << frames[i].c_str() << endl;
			return 1;
		}

		CImage currImage( bmpDataCurrFrame );
		CImage prevImage( bmpDataPrevFrame );
		// —читаем глобальный вектор перемещени€ между предыдущим и текущим кадром
		time_t start = clock();
		CVector globalVector = currImage.EstimateMotionVectorFrom( prevImage );
		time_t finish = clock();
		// wcout << ( double ) ( finish - start ) / CLOCKS_PER_SEC << endl;
		// ¬ектор смещени€ относительно самого первого кадра получаетс€ сложением 
		// смещений предыдущих кадров
		accumulatedVector += globalVector;
		wcout << accumulatedVector.X << L" " << accumulatedVector.Y << endl;

		currentFrame.UnlockBits( &bmpDataCurrFrame );
		previousFrame.UnlockBits( &bmpDataPrevFrame );
	}

	GdiplusShutdown( gdiplusToken );

	return 0;
}
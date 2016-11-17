#include <Common.h>
#pragma hdrstop

#include <Image.h>
#include <ctime>
#include <fstream>

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

void CompareImages( const TImage& first, const TImage& second )
{
	for( size_t y = 0; y < first.size(); y++ ) {
		for( size_t x = 0; x < first[y].size(); x++ ) {
			if( first[y][x] != second[y][x] ) {
				DebugBreak();
			}
		}
	}
}

int wmain( int argc, wchar_t* argv[] )
{
	if( argc != 2 ) {
		wcout << L"Usage: <inputFile>" << endl;
		return 1;
	}

	wchar_t* source = argv[1];

	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	GdiplusStartup( &gdiplusToken, &gdiplusStartupInput, 0 );

	{
		Bitmap GDIBitmapSource( source );
		int w = GDIBitmapSource.GetWidth();
		int h = GDIBitmapSource.GetHeight();
		BitmapData bmpDataSource;
		// Whole image
		Rect rc( 0, 0, w, h );
		if( Ok != GDIBitmapSource.LockBits( &rc, ImageLockModeRead | ImageLockModeWrite, PixelFormat24bppRGB, &bmpDataSource ) ) {
			wcout << L"Failed to lock image: " << source << endl;
			return 1;
		} else {
			wcout << L"Source file:" << source << endl;
		}

		/*size_t filterRadius;
		wcin >> filterRadius;*/

		
		CImage img( bmpDataSource );
		for( size_t filterRadius = 1; filterRadius < 102; filterRadius += 10 ) {
			TImage hgw;
			size_t start = clock();
			img.HGWDilate( hgw, filterRadius );
			size_t finish = clock();
			// todo: write in file via fstream
		}
		
		GDIBitmapSource.UnlockBits( &bmpDataSource );
	}

	GdiplusShutdown( gdiplusToken );

	return 0;
}
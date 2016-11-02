#include <Common.h>
#include <HGWMorphology.h>

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
	if( argc != 3 ) {
		wcout << L"Usage: <inputFile.bmp> <outputFile.bmp>" << endl;
		return 1;
	}

	wchar_t* source = argv[1];
	wchar_t* destination = argv[2];

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
			wcout << L"Failed to lock image: " << argv[1] << endl;
			return 1;
		} else {
			wcout << L"Source file:" << argv[1] << endl;
		}

		size_t filterSize = 3;
		//wcin >> filterSize;
		// Processing
		MorphOp( bmpDataSource, filterSize );
	
		GDIBitmapSource.UnlockBits( &bmpDataSource );

		// Save result
		CLSID clsId;
		GetEncoderClsid( L"image/bmp", &clsId );
		GDIBitmapSource.Save( destination, &clsId, 0 );
	}

	GdiplusShutdown( gdiplusToken );

	return 0;
}
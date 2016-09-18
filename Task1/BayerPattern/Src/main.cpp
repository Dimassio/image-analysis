#include <Common.h>
#include <BayerPattern.h>

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
		cout << L"Usage: BayerPattern <inputFile.bmp> <outputFile.bmp>" << endl;
		return 0;
	}

	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	GdiplusStartup( &gdiplusToken, &gdiplusStartupInput, 0 );
	
	{
		Bitmap GDIBitmap( argv[1] );
		int w = GDIBitmap.GetWidth();
		int h = GDIBitmap.GetHeight();
		BitmapData bmpData;
		Rect rc( 0, 0, w, h ); // whole image
		if( Ok != GDIBitmap.LockBits( &rc, ImageLockModeRead | ImageLockModeWrite, PixelFormat24bppRGB, &bmpData ) ) {
			cout << L"Failed to lock image: " << argv[1] << endl;
		} else {
			cout << L"File: %s\n" << argv[1] << endl;
		}

		CBayerPattern image( bmpData );
		image.Process();
		image.GetData( bmpData );

		GDIBitmap.UnlockBits( &bmpData );

		// Save result
		CLSID clsId;
		GetEncoderClsid( L"image/bmp", &clsId );
		GDIBitmap.Save( argv[2], &clsId, 0 );
	}

	GdiplusShutdown( gdiplusToken );

	return 0;
}
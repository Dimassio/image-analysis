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


void CompareImages( const BitmapData& source, const BitmapData& original )
{
	const size_t h = source.Height;
	const size_t w = source.Width;
	const int bpr = original.Stride;
	const int bpp = 3; // BGR24
	BYTE* pBuffer = ( BYTE* ) original.Scan0;
	BYTE* pBuffer2 = ( BYTE* ) source.Scan0;
	int baseAdr = 0;
	for( size_t y = 0; y < h; y++ ) {
		int pixelAdr = baseAdr;
		for( size_t x = 0; x < w; x++ ) {
			BYTE oneByte = pBuffer[pixelAdr];
			BYTE secByte = pBuffer2[pixelAdr];
			if( oneByte !=  secByte ) {
				wcout << x << " " << y << endl;
			}
			pixelAdr += bpp;
		}
		wcout << endl;
		baseAdr += bpr;
	}
}

int wmain( int argc, wchar_t* argv[] )
{
	if( argc != 4 ) {
		wcout << L"Usage: <inputFile> <original> <outputFile>" << endl;
		return 1;
	}

	wchar_t* source = argv[1];
	wchar_t* original = argv[2];
	wchar_t* destination = argv[3];

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

		Bitmap GDIBitmapOriginal( original );
		assert( w == GDIBitmapOriginal.GetWidth() );
		assert( h == GDIBitmapOriginal.GetHeight() );
		BitmapData bmpDataOriginal;
		if( Ok != GDIBitmapOriginal.LockBits( &rc, ImageLockModeRead | ImageLockModeWrite, PixelFormat24bppRGB, &bmpDataOriginal ) ) {
			wcout << L"Failed to lock image: "  << original << endl;
			return 1;
		} else {
			wcout << L"Original file:" << original << endl;
		}
			
		size_t filterRadius;
		wcin >> filterRadius;
		// Processing
		MorphOp( bmpDataSource, filterRadius );
		
		// Save result
		CLSID clsId;
		GetEncoderClsid( L"image/jpg", &clsId );
		GDIBitmapSource.Save( destination, &clsId, 0 ); 

		GDIBitmapSource.UnlockBits( &bmpDataSource );
		GDIBitmapOriginal.UnlockBits( &bmpDataOriginal );
	}

	GdiplusShutdown( gdiplusToken );

	return 0;
}
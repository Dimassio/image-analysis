#include <Common.h>
#pragma hdrstop

#include <Image.h>

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

void GetData( const TImage& image, BitmapData& bmpData )
{
	const int bpr = bmpData.Stride;
	const int bpp = 3; // BGR24
	size_t width = bmpData.Width;
	size_t height = bmpData.Height;
	BYTE* pBuffer = ( BYTE* ) bmpData.Scan0;
	int baseAdr = 0;
	for( size_t y = 0; y < height; y++ ) {
		int pixelAdr = baseAdr;
		for( size_t x = 0; x < width; x++ ) {
			pBuffer[pixelAdr] = image[y][x];
			pBuffer[pixelAdr + 1] = image[y][x];
			pBuffer[pixelAdr + 2] = image[y][x];
			pixelAdr += bpp;
		}
		baseAdr += bpr;
	}

}

void CompareImages( const TImage& first, const TImage& second )
{
	bool done = false;
	bool succeeded = true;
	for( size_t y = 0; y < first.size(); y++ ) {
		for( size_t x = 0; x < first[y].size(); x++ ) {
			if( first[y][x] != second[y][x] ) {
				wcout << "Not equal" << endl;
				wcout << y << " " << x << endl;
				done = true;
				succeeded = false;
				break;
			}
		}
		if( done ) {
			break;
		}
	}
	if( succeeded ) {
		wcout << "Equal!" << endl;
	}
}

int wmain( int argc, wchar_t* argv[] )
{
	if( argc != 3 ) {
		wcout << L"Usage: <inputFile> <outputFile>" << endl;
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
			wcout << L"Failed to lock image: " << source << endl;
			return 1;
		} else {
			wcout << L"Source file:" << source << endl;
		}

		size_t filterRadius;
		wcin >> filterRadius;
		CImage img( bmpDataSource, MO_Dilate );
		TImage dilatedImage;
		img.HGWMorph( dilatedImage, filterRadius );
		GetData( dilatedImage, bmpDataSource );

		GDIBitmapSource.UnlockBits( &bmpDataSource );

		// Save result
		CLSID clsId;
		GetEncoderClsid( L"image/bmp", &clsId );
		GDIBitmapSource.Save( destination, &clsId, 0 );
	}

	GdiplusShutdown( gdiplusToken );

	return 0;
}
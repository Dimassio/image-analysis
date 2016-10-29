#include <Common.h>
#include <BayerPattern.h>

static const int LumaRed = 9798;
static const int LumaGreen = 19235;
static const int LumaBlue = 3735;
static const int MaximumFluctuation = 255;
static const int CoeffNormalizationBitsCount = 15;
static const int CoeffNormalization = 1 << CoeffNormalizationBitsCount;

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

double getPeakSignalToNoizeRatio( const BitmapData& noizedImage, const BitmapData& originalImage )
{
	const size_t w = noizedImage.Width;
	const size_t h = noizedImage.Height;
	const int bpr = noizedImage.Stride;
	const int bpp = 3; // BGR24
	BYTE* pBufferNoized = ( BYTE* ) noizedImage.Scan0;
	BYTE* pBufferOriginal = ( BYTE* ) originalImage.Scan0;
	double PSNR = 0.0;
	double MSE = 0.0;
	int baseAdr = 0;
	int numOfPixels = 0;
	for( size_t y = 0; y < h; ++y ) {
		int pixelAdr = baseAdr;
		for( size_t x = 0; x < w; ++x ) {
			int BNoized = pBufferNoized[pixelAdr]; // blue
			int GNoized = pBufferNoized[pixelAdr + 1]; // green
			int RNoized = pBufferNoized[pixelAdr + 2]; // red

			int BOriginal = pBufferOriginal[pixelAdr]; // blue
			int GOriginal = pBufferOriginal[pixelAdr + 1]; // green
			int ROriginal = pBufferOriginal[pixelAdr + 2]; // red

			int YNoized = ( LumaRed * RNoized + LumaGreen * GNoized + LumaBlue * BNoized + ( CoeffNormalization >> 1 ) ) >> CoeffNormalizationBitsCount;
			int YOriginal = ( LumaRed * ROriginal + LumaGreen * GOriginal + LumaBlue * BOriginal + ( CoeffNormalization >> 1 ) ) >> CoeffNormalizationBitsCount;
			MSE += ( YNoized - YOriginal ) * ( YNoized - YOriginal );
			++numOfPixels;

			pixelAdr += bpp;
		}
		baseAdr += bpr;
	}

	MSE /= numOfPixels;
	wcout << L"MSE is " << MSE << endl;
	PSNR = 10 * log10( MaximumFluctuation * MaximumFluctuation / MSE );

	return PSNR;
}

int wmain( int argc, wchar_t* argv[] )
{
	if( argc != 4 ) {
		wcout << L"Usage: BayerPattern <inputFile.bmp> <outputFile.bmp> <originalImage.bmp>" << endl;
		return 1;
	}

	wchar_t* source = argv[1];
	wchar_t* destination = argv[2];
	wchar_t* original = argv[3];

	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	GdiplusStartup( &gdiplusToken, &gdiplusStartupInput, 0 );

	{
		Bitmap GDIBitmapSource( source );
		Bitmap GDIBitmapOriginal( original );
		int w = GDIBitmapSource.GetWidth();
		int h = GDIBitmapSource.GetHeight();

		assert( w == GDIBitmapOriginal.GetWidth() && h == GDIBitmapOriginal.GetHeight() );

		BitmapData bmpDataSource;
		BitmapData bmpDataOriginal;
		// Whole image
		Rect rc( 0, 0, w, h );
		// Для исходного
		if( Ok != GDIBitmapSource.LockBits( &rc, ImageLockModeRead | ImageLockModeWrite, PixelFormat24bppRGB, &bmpDataSource ) ) {
			wcout << L"Failed to lock image: " << argv[1] << endl;
			return 1;
		} else {
			wcout << L"Source file:" << argv[1] << endl;
		}
		// Для оригинального изображения
		if( Ok != GDIBitmapOriginal.LockBits( &rc, ImageLockModeRead | ImageLockModeWrite, PixelFormat24bppRGB, &bmpDataOriginal ) ) {
			wcout << L"Failed to lock image: " << argv[3] << endl;
			return 1;
		} else {
			wcout << L"Original file:" << argv[3] << endl;
		}

		// Восстанавливаем изображение
		CBayerPattern image( bmpDataSource );
		image.Process();
		wcout << L"Processing done" << endl;
		image.GetData( bmpDataSource );

		// Считаем качество
		wcout << L"Calculating PSNR..." << endl;
		double PSNR = getPeakSignalToNoizeRatio( bmpDataSource, bmpDataOriginal );
		wcout << L"PSNR is: " << PSNR << endl;

		GDIBitmapOriginal.UnlockBits( &bmpDataOriginal );
		GDIBitmapSource.UnlockBits( &bmpDataSource );

		// Save result
		CLSID clsId;
		GetEncoderClsid( L"image/bmp", &clsId );
		GDIBitmapSource.Save( destination, &clsId, 0 );
	}

	GdiplusShutdown( gdiplusToken );

	return 0;
}
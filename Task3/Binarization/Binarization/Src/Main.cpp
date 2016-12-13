#include <Common.h>
#pragma hdrstop

#include <Image.h>
#include <fstream>
#include <deque>

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

void initializeTempBuffer( const std::vector< std::vector<BYTE> >& image, std::vector<int>& sumOfPixels, std::deque<std::vector<int>>& temp )
{
	int filterSize = temp.size();
	int zeroLevel = temp.size() / 2;
	int n = sumOfPixels.size();
	// Середина temp - это первая строка изображения, то есть zeroLevel-тая строка
	// в sumOfPixels уже будут необходимые суммы по первой строке изображения
	for( size_t i = 0; i < temp.size(); ++i ) {
		// 1ую просто заполняем
		int sum = 0;
		for( size_t j = 0; j < filterSize; ++j ) {
			sum += image[i][j];
		}
		temp[i][0] = sum;
		// А дальше идем скользящим окном
		for( size_t j = zeroLevel + 1; j < zeroLevel + n; ++j ) {
			sum = sum - image[i][j - zeroLevel - 1] + image[i][j + zeroLevel];
			temp[i][j - zeroLevel] = sum;
		}
	}

	for( size_t j = 0; j < temp[0].size(); ++j ) {
		int sum = 0;
		for( size_t i = 0; i < temp.size(); ++i ) {
			sum += temp[i][j];
		}
		sumOfPixels[j] = sum;
	}
}

void fillLeftRightEdges( int zeroLevel, int n, std::vector< std::vector<BYTE> >& image )
{
	for( size_t i = 0; i < image.size(); ++i ) {
		// left edge
		for( size_t j = 0; j < zeroLevel; ++j ) {
			image[i][j] = image[i][zeroLevel];
		}
		// right edge
		for( size_t j = n + zeroLevel; j < image.size(); ++j ) {
			image[i][j] = image[i][zeroLevel + n - 1];
		}
	}
}

void fillUpperAndLowerEdges( int zeroLevel, int n, std::vector< std::vector<BYTE> >& image )
{
	for( size_t x = zeroLevel; x < zeroLevel + n; ++x ) {
		for( size_t y = 0; y < zeroLevel; ++y ) {
			image[y][x] = image[zeroLevel][x];
		}
		for( size_t y = zeroLevel + n; y < image.size(); ++y ) {
			image[y][x] = image[zeroLevel + n - 1][x];
		}
	}
}

void printSum( const std::vector<int>& sumOfPixels )
{
	for( size_t i = 0; i < sumOfPixels.size(); ++i ) {
		wcout << sumOfPixels[i] << " ";
	}
	wcout << endl << endl; 
}

bool doTest()
{
	std::ifstream fin( "input.txt", std::ios::in );
	std::vector<std::vector<BYTE>> image;
	int filterSize = 5;
	int zeroLevel = filterSize / 2;
	size_t n;
	fin >> n;
	image.resize( n + 2 * zeroLevel );
	for( size_t y = 0; y < image.size(); y++ ) {
		image[y].resize( n + 2 * zeroLevel );
	}

	for( size_t i = 0; i < n; ++i ) {
		for( size_t j = 0; j < n; ++j ) {
			int value;
			fin >> value;
			image[i + zeroLevel][j + zeroLevel] = value;
		}
	}
	fillUpperAndLowerEdges( zeroLevel, n, image );
	fillLeftRightEdges( zeroLevel, n, image );
	for( size_t i = 0; i < image.size(); ++i ) {
		for( size_t j = 0; j < image[i].size(); ++j ) {
			wcout << image[i][j] << " ";
		}
		wcout << endl;
	}
	wcout << endl << endl;


	// ALGORITHM
	std::vector<int> sumOfPixels( n );
	std::deque< std::vector<int> > tmp( filterSize );
	for( size_t i = 0; i < filterSize; ++i ) {
		tmp[i].resize( n );
	}
	initializeTempBuffer( image, sumOfPixels, tmp );
	printSum( sumOfPixels ); // ++++++++++

	// Начинаем с +1, так как первую линию уже сделали
	for( size_t i = zeroLevel + 1; i < zeroLevel + n; ++i ) {
		std::vector<int> newLine( n );
		// 1ую просто заполняем
		int sum = 0;
		for( size_t j = 0; j < filterSize; ++j ) {
			sum += image[i + 1][j];
		}
		newLine[0] = sum;
		sumOfPixels[0] = sumOfPixels[0] - tmp[0][0] + newLine[0];
		for( size_t j = zeroLevel + 1; j < zeroLevel + n; ++j ) {
			sum = sum - image[i + 1][j - zeroLevel - 1] + image[i + 1][j + zeroLevel];
			newLine[j - zeroLevel] = sum;
			sumOfPixels[j - zeroLevel] = sumOfPixels[j - zeroLevel] - tmp[0][j - zeroLevel] + newLine[j - zeroLevel];
		}
		
		tmp.push_back( newLine );
		tmp.pop_front();

		printSum( sumOfPixels ); //+++++++++++++++++++
	}

	fin.close();
	return true;
}

int wmain( int argc, wchar_t* argv[] )
{
	if( doTest() ) {
		return 0;
	}
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

		CImage img( bmpDataSource );
		img.NICKBinarization();
		img.GetData( bmpDataSource );

		GDIBitmapSource.UnlockBits( &bmpDataSource );

		// Save result
		CLSID clsId;
		GetEncoderClsid( L"image/jpeg", &clsId );
		GDIBitmapSource.Save( destination, &clsId, 0 );
	}

	GdiplusShutdown( gdiplusToken );

	return 0;
}
#include <Common.h>
#pragma hdrstop

#include <Image.h>

// Окна 19 на 19
const int Radius = 9;

CImage::CImage( const BitmapData& bmpData )
{
	zeroLevel = 9;
	const int bpr = bmpData.Stride;
	const int bpp = 3; // BGR24
	BYTE* pBuffer = ( BYTE* ) bmpData.Scan0;
	width = bmpData.Width;
	height = bmpData.Height;

	binarizedImage.resize( height );
	//A.resize( height );
	for( size_t y = 0; y < binarizedImage.size(); ++y ) {
		binarizedImage[y].resize( width );
		//A[y].resize( width );
	}

	image.resize( height + 2 * zeroLevel );
	for( size_t y = 0; y < image.size(); y++ ) {
		image[y].resize( width + 2 * zeroLevel );
	}

	int baseAdr = 0;
	for( size_t y = 0; y < height; y++ ) {
		int pixelAdr = baseAdr;
		for( size_t x = 0; x < width; x++ ) {
			BYTE B = pBuffer[pixelAdr]; // blue
			BYTE G = pBuffer[pixelAdr + 1]; // green
			BYTE R = pBuffer[pixelAdr + 2]; // red
			image[zeroLevel + y][zeroLevel + x] = ( int ) ( 0.2125 * R + 0.7154 * G + 0.0721 * B );
			pixelAdr += bpp;
		}
		baseAdr += bpr;
	}

	fillUpperAndLowerEdges();
	fillLeftRightEdges();
}

void CImage::GetData( BitmapData& bmpData ) const
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
			pBuffer[pixelAdr] = binarizedImage[y][x];
			pBuffer[pixelAdr + 1] = binarizedImage[y][x];
			pBuffer[pixelAdr + 2] = binarizedImage[y][x];
			pixelAdr += bpp;
		}
		baseAdr += bpr;
	}
}

void CImage::NICKBinarization()
{
	for( size_t i = zeroLevel; i < zeroLevel + height; ++i ) {
		wcout << i << endl;

		for( size_t j = zeroLevel; j < zeroLevel + width; ++j ) {
			BYTE t = getWindowthreshold( i, j );
			if( image[i][j] > t ) {
				binarizedImage[i - zeroLevel][j - zeroLevel] = 255;
			} else {
				binarizedImage[i - zeroLevel][j - zeroLevel] = 0;
			}
		}
	}
}

BYTE CImage::getWindowthreshold( int _i, int _j ) const
{
	int left = max( _j - Radius, 0 );
	int top = max( _i - Radius, 0 );
	int right = min( width - 1, _j + Radius ) + 1;
	int bottom = min( height - 1, _i + Radius ) + 1;

	// k try from -0.2 to -0.1.
	double k = -0.1;
	int sumOfSqrPixels = 0;
	int sumOfPixels = 0;
	int numberOfPixels = ( 2 * Radius + 1 ) * ( 2 * Radius + 1 );
	for( int i = top; i < bottom; ++i ) {
		for( int j = left; j < right; ++j ) {
			sumOfPixels += image[i][j];
			sumOfSqrPixels += image[i][j] * image[i][j];
		}
	}
	BYTE mean = sumOfPixels / numberOfPixels;
	BYTE result = mean + k * sqrt( ( sumOfSqrPixels - mean * mean ) / numberOfPixels );
	return result;
}

void CImage::fillLeftRightEdges()
{
	for( size_t i = 0; i < image.size(); ++i ) {
		// left edge
		for( size_t j = 0; j < zeroLevel; ++j ) {
			image[i][j] = image[i][zeroLevel];
		}
		// right edge
		for( size_t j = width + zeroLevel; j < image.size(); ++j ) {
			image[i][j] = image[i][zeroLevel + width - 1];
		}
	}
}

void CImage::fillUpperAndLowerEdges()
{
	for( size_t x = zeroLevel; x < zeroLevel + width; ++x ) {
		for( size_t y = 0; y < zeroLevel; ++y ) {
			image[y][x] = image[zeroLevel][x];
		}
		for( size_t y = zeroLevel + height; y < image.size(); ++y ) {
			image[y][x] = image[zeroLevel + height - 1][x];
		}
	}
}

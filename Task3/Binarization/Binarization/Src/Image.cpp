#include <Common.h>
#pragma hdrstop

#include <Image.h>

CImage::CImage( const BitmapData& bmpData )
{
	zeroLevel = Radius;
	filterSize = 2 * Radius + 1;
	const int bpr = bmpData.Stride;
	const int bpp = 3; // BGR24
	BYTE* pBuffer = ( BYTE* ) bmpData.Scan0;
	width = bmpData.Width;
	height = bmpData.Height;

	binarizedImage.resize( height );
	for( size_t y = 0; y < binarizedImage.size(); ++y ) {
		binarizedImage[y].resize( width );
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
	const int D0 = 120;
	int numberOfPixels = filterSize * filterSize;

	std::vector<int> sumOfPixels;
	std::vector<int> sumOfSqrPixels;
	std::deque< std::vector<int> > tempSum;
	std::deque< std::vector<int> > tempSqrSum;
	initializeTempBuffers( sumOfPixels, tempSum, sumOfSqrPixels, tempSqrSum );

	for( size_t i = zeroLevel; i < zeroLevel + height; ++i ) {
		if( i == zeroLevel ) {
			// Первая строка уже предподсчитана
			for( size_t j = zeroLevel; j < zeroLevel + width; ++j ) {
				double mean = sumOfPixels[j - zeroLevel] / numberOfPixels;

				double D = sumOfSqrPixels[j - zeroLevel] / numberOfPixels - ( mean ) * ( mean );
				double k = D > D0 ? -0.1 : -0.2;

				BYTE threshold = mean + k * sqrt( ( sumOfSqrPixels[j - zeroLevel] - mean * mean ) / numberOfPixels );
				binarizePixel( i, j, threshold );

			}
			continue;
		}
		std::vector<int> newLine( width );
		std::vector<int> newSqrLine( width );
		// 1ую ячейку заполняем
		int sum = 0;
		int sqrSum = 0;
		for( size_t j = 0; j < filterSize; ++j ) {
			sum += image[i + 1][j];
			sqrSum += image[i + 1][j] * image[i + 1][j];
		}
		newLine[0] = sum;
		newSqrLine[0] = sqrSum;
		sumOfPixels[0] = sumOfPixels[0] - tempSum[0][0] + newLine[0];
		sumOfSqrPixels[0] = sumOfSqrPixels[0] - tempSqrSum[0][0] + newSqrLine[0];
		for( size_t j = zeroLevel + 1; j < zeroLevel + width; ++j ) {
			sum = sum - image[i + 1][j - zeroLevel - 1] + image[i + 1][j + zeroLevel];
			sqrSum = sqrSum - image[i + 1][j - zeroLevel - 1] * image[i + 1][j - zeroLevel - 1] +
				image[i + 1][j + zeroLevel] * image[i + 1][j + zeroLevel];
			newLine[j - zeroLevel] = sum;
			newSqrLine[j - zeroLevel] = sqrSum;
			sumOfPixels[j - zeroLevel] = sumOfPixels[j - zeroLevel] - tempSum[0][j - zeroLevel] + newLine[j - zeroLevel];
			sumOfSqrPixels[j - zeroLevel] = sumOfSqrPixels[j - zeroLevel] - tempSqrSum[0][j - zeroLevel] + newSqrLine[j - zeroLevel];

			double mean = sumOfPixels[j - zeroLevel] / numberOfPixels;

			double D = sumOfSqrPixels[j - zeroLevel] / numberOfPixels - ( mean ) * ( mean );
			double k = D > D0 ? -0.1 : -0.2;
			BYTE threshold = mean + k * sqrt( ( sumOfSqrPixels[j - zeroLevel] - mean * mean ) / numberOfPixels );
			binarizePixel( i, j, threshold );
		}

		tempSum.push_back( newLine );
		tempSum.pop_front();
		tempSqrSum.push_back( newSqrLine );
		tempSqrSum.pop_front();

		if( i % 100 == 0 ) {
			wcout << i << endl;
		}
	}
}

void CImage::initializeTempBuffers( std::vector<int>& sumOfPixels, std::deque< std::vector<int> >& tempSum,
									std::vector<int>& sumOfSqrPixels, std::deque< std::vector<int> >& tempSqrSum ) const
{
	sumOfPixels.resize( width, 0 );
	sumOfSqrPixels.resize( width, 0 );
	tempSum.resize( filterSize );
	tempSqrSum.resize( filterSize );
	for( size_t i = 0; i < filterSize; ++i ) {
		tempSum[i].resize( width );
		tempSqrSum[i].resize( width );
	}
	// Середина temp - это первая строка изображения
	// в sumOfPixels уже будут необходимые суммы по первой строке изображения
	for( size_t i = 0; i < filterSize; ++i ) {
		// 1ую просто заполняем
		int sum = 0;
		int sqrSum = 0;
		for( size_t j = 0; j < filterSize; ++j ) {
			sum += image[i][j];
			sqrSum += image[i][j] * image[i][j];
		}
		tempSum[i][0] = sum;
		tempSqrSum[i][0] = sqrSum;
		sumOfPixels[0] += sum;
		sumOfSqrPixels[0] += sqrSum;
		// А дальше идем скользящим окном
		for( size_t j = zeroLevel + 1; j < zeroLevel + width; ++j ) {
			sum = sum - image[i][j - zeroLevel - 1] + image[i][j + zeroLevel];
			sqrSum = sqrSum - image[i][j - zeroLevel - 1] * image[i][j - zeroLevel - 1] +
				image[i][j + zeroLevel] * image[i][j + zeroLevel];
			tempSum[i][j - zeroLevel] = sum;
			tempSqrSum[i][j - zeroLevel] = sqrSum;
			// Заполняем суммы по вертикали
			sumOfPixels[j - zeroLevel] += sum;
			sumOfSqrPixels[j - zeroLevel] += sqrSum;
		}
	}
}

void CImage::binarizePixel( size_t i, size_t j, BYTE threshold )
{
	if( image[i][j] > threshold ) {
		binarizedImage[i - zeroLevel][j - zeroLevel] = 255;
	} else {
		binarizedImage[i - zeroLevel][j - zeroLevel] = 0;
	}
}

void CImage::fillLeftRightEdges()
{
	for( size_t i = 0; i < image.size(); ++i ) {
		// left edge
		for( size_t j = 0; j < zeroLevel; ++j ) {
			image[i][j] = image[i][zeroLevel];
		}
		// right edge
		for( size_t j = width + zeroLevel; j < image[i].size(); ++j ) {
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

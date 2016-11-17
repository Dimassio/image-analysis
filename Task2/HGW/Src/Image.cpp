#include <Common.h>
#pragma hdrstop

#include <Image.h>

CImage::CImage( const BitmapData& bmpData )
{
	const int bpr = bmpData.Stride;
	const int bpp = 3; // BGR24
	BYTE* pBuffer = ( BYTE* ) bmpData.Scan0;
	width = bmpData.Width;
	height = bmpData.Height;
	image.resize( height );
	int baseAdr = 0;
	for( size_t y = 0; y < height; y++ ) {
		image[y].resize( width );
		int pixelAdr = baseAdr;
		for( size_t x = 0; x < width; x++ ) {
			image[y][x] = pBuffer[pixelAdr];
			pixelAdr += bpp;
		}
		baseAdr += bpr;
	}
}

void CImage::EasyDilate( TImage& dilated, const size_t radius ) const
{
	dilated.resize( image.size() );
	for( size_t i = 0; i < image.size(); ++i ) {
		dilated[i].resize( image[i].size() );
		for( size_t j = 0; j < image[i].size(); ++j ) {
			dilated[i][j] = getMaxValue( i, j, radius );
		}
	}
}

void CImage::HGWDilate( TImage& result, const size_t radius ) const
{
	// 1 проход по вертикали
	// wcout << "First trace..." << endl;
	TImage dilatedFirstTime;
	hgwProcessing( image, radius, dilatedFirstTime );
	//wcout << "Transposition..." << endl;
	// Транспонируем
	TImage imageTr;
	imageTr.resize( width );
	for( size_t i = 0; i < width; ++i ) {
		for( size_t j = 0; j < height; ++j ) {
			imageTr[i].push_back( dilatedFirstTime[j][i] );
		}
	}
	// 2 проход по горизонтали
	//wcout << "Second trace..." << endl;
	TImage dilatedSecondTime;
	hgwProcessing( imageTr, radius, dilatedSecondTime );
	/* Для тестирования нужно закомментировать кусок ниже */
	result.resize( height );
	for( size_t i = 0; i < height; ++i ) {
		for( size_t j = 0; j < width; ++j ) {
			result[i].push_back( dilatedSecondTime[j][i] );
		}
	}
}

void CImage::GetData( BitmapData& bmpData ) const
{
}

void CImage::hgwProcessing( const TImage& _image, const size_t filterSize, TImage& result ) const
{
	result.resize( _image.size() );
	for( size_t i = 0; i < result.size(); ++i ) {
		oneDimensionalHGW( _image[i], filterSize, result[i] );
	}
}

void CImage::oneDimensionalHGW( const std::vector<BYTE>& a, const size_t radius, std::vector<BYTE>& result ) const
{
	const int size = 2 * radius + 1;
	result.resize( a.size() );
	std::vector<BYTE> c( size - 1 );
	std::vector<BYTE> d( size - 1 );
	for( int center = radius; center < a.size() + radius; center += ( size - 1 ) ) {
		if( center >= a.size() ) {
			d[0] = 0;
		} else {
			d[0] = a[center];
		}
		for( int i = 1; i < size - 1; ++i ) {
			if( center + i >= a.size() ) {
				d[i] = max( d[i - 1], 0 );
			} else {
				d[i] = max( d[i - 1], a[center + i] );
			}
		}

		if( center - 1 >= a.size() ) {
			c[size - 2] = 0;
		} else {
			c[size - 2] = a[center - 1];
		}
		for( int i = 1; i < size - 1; ++i ) {
			if( ( center - i - 1 ) < 0 || ( center - i - 1 ) >= a.size() ) {
				c[size - i - 2] = max( c[size - i - 1], 0 );
			} else {
				c[size - i - 2] = max( c[size - i - 1], a[center - i - 1] );
			}
		}

		for( int i = 0; i < size - 1; ++i ) {
			if( ( center - radius + i ) < a.size() ) {
				result[center - radius + i] = max( c[i], d[i] );
			}
		}
	}
}

BYTE CImage::getMaxValue( int i, int j, int rad ) const
{
	int left = max( j - rad, 0 );
	int top = max( i - rad, 0 );
	int right = min( image[0].size() - 1, j + rad ) + 1;
	int bottom = min( image.size() - 1, i + rad ) + 1;

	int maximum = 0;
	for( int ii = top; ii < bottom; ++ii ) {
		for( int jj = left; jj < right; ++jj ) {
			maximum = max( maximum, image[ii][jj] );
		}
	}
	return maximum;
}
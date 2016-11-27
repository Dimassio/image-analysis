#include <Common.h>
#pragma hdrstop

#include <Image.h>

CImage::CImage( const BitmapData& bmpData, TMorphOp op ):
	currOp( op )
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

void CImage::EasyMorph( TImage& dilated, const size_t radius ) const
{
	dilated.resize( image.size() );
	for( size_t i = 0; i < image.size(); ++i ) {
		dilated[i].resize( image[i].size() );
		for( size_t j = 0; j < image[i].size(); ++j ) {
			dilated[i][j] = getMorphValue( i, j, radius );
		}
	}
}

void CImage::HGWMorph( TImage& result, const size_t radius ) const
{
	// 1 проход по вертикали
	wcout << "First trace..." << endl;
	TImage dilatedFirstTime;
	hgwProcessing( image, radius, dilatedFirstTime );
	wcout << "Transposition..." << endl;
	// Транспонируем
	TImage imageTr;
	imageTr.resize( width );
	for( size_t i = 0; i < width; ++i ) {
		for( size_t j = 0; j < height; ++j ) {
			imageTr[i].push_back( dilatedFirstTime[j][i] );
		}
	}
	// 2 проход по горизонтали
	wcout << "Second trace..." << endl;
	TImage dilatedSecondTime;
	hgwProcessing( imageTr, radius, dilatedSecondTime );
	// Обратно транспонируем
	result.resize( height );
	for( size_t i = 0; i < height; ++i ) {
		for( size_t j = 0; j < width; ++j ) {
			result[i].push_back( dilatedSecondTime[j][i] );
		}
	}
}

void CImage::hgwProcessing( const TImage& _image, const size_t filterSize, TImage& result ) const
{
	result.resize( _image.size() );
	for( size_t i = 0; i < result.size(); ++i ) {
		oneDimensionalHGW( _image[i], filterSize, result[i] );
	}
}

void CImage::oneDimensionalHGW( const std::vector<BYTE>& input, const size_t radius, std::vector<BYTE>& result ) const
{
	const int size = 2 * radius + 1;
	size_t n = input.size();
	result.resize( n );
	std::vector<BYTE> c( size - 1 );
	std::vector<BYTE> d( size - 1 );
	for( int center = radius; center < n + radius; center += ( size - 1 ) ) {
		if( center >= n ) {
			d[0] = input[n - 1];
		} else {
			d[0] = input[center];
		}
		for( int i = 1; i < size - 1; ++i ) {
			if( center + i >= n ) {
				d[i] = calculateMorphValue( d[i - 1], input[n - 1] );
			} else {
				d[i] = calculateMorphValue( d[i - 1], input[center + i] );
			}
		}

		if( center - 1 >= n ) {
			c[size - 2] = input[n - 1];
		} else {
			c[size - 2] = input[center - 1];
		}
		for( int i = 1; i < size - 1; ++i ) {
			if( ( center - i - 1 ) < 0 ) {
				c[size - i - 2] = calculateMorphValue( c[size - i - 1], input[0] );
			} else if( ( center - i - 1 ) >= n ) {
				c[size - i - 2] = calculateMorphValue( c[size - i - 1], input[n - 1] );
			} else {
				c[size - i - 2] = calculateMorphValue( c[size - i - 1], input[center - i - 1] );
			}
		}

		for( size_t i = 0; i < size - 1; ++i ) {
			if( ( center - radius + i ) < n ) {
				result[center - radius + i] = calculateMorphValue( c[i], d[i] );
			}
		}
	}
}

BYTE CImage::calculateMorphValue( BYTE firstValue, BYTE secondValue ) const
{
	BYTE result = 0;
	switch( currOp ) {
		case MO_Dilate:
			result = max( firstValue, secondValue );
			break;
		case MO_Erase:
			result = min( firstValue, secondValue );
			break;
		default:
			assert( false );
	}
	return result;
}

BYTE CImage::getMorphValue( int _i, int _j, int rad ) const
{
	int left = max( _j - rad, 0 );
	int top = max( _i - rad, 0 );
	int right = min( width - 1, _j + rad ) + 1;
	int bottom = min( height - 1, _i + rad ) + 1;
	int result = image[top][left];
	for( int i = top; i < bottom; ++i ) {
		for( int j = left; j < right; ++j ) {
			result = calculateMorphValue( result, image[i][j] );
		}
	}
	return result;
}
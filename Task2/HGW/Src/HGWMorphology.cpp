#include <Common.h>
#include <HGWMorphology.h>
#include <cstdlib>

std::vector<BYTE> calculateHGW( const std::vector<BYTE>& a, const size_t filterSize )
{
	const size_t s = 2 * filterSize + 1;
	std::vector<BYTE> b( a.size() );
	std::vector<BYTE> c( s - 1 );
	std::vector<BYTE> d( s - 1 );
	// + filter - для корректной обработки на краях
	for( size_t center = filterSize; center < a.size() + filterSize; center += ( s - 1 ) ) {
		if( center >= a.size() ) {
			d[0] = 0;
		} else {
			d[0] = a[center];
		}
		for( size_t i = 1; i < s - 1; ++i ) {
			if( center + i >= a.size() ) {
				d[i] = max( d[i - 1], 0 );
			} else {
				d[i] = max( d[i - 1], a[center + i] );
			}
		}

		if( center - 1 >= a.size() ) {
			c[s - 2] = 0;
		} else {
			c[s - 2] = a[center - 1];
		}
		for( size_t i = 1; i < s - 1; ++i ) {
			if( ( int ) (center - 1 - i ) < 0 || ( center - 1 - i ) >= a.size() ) {
				c[s - 2 - i] = max( c[s - 1 - i], 0 );
			} else {
				c[s - 2 - i] = max( c[s - 1 - i], a[center - 1 - i] );
			}
		}

		for( size_t i = 0; i < s - 1; ++i ) {
			if( ( center - filterSize + i ) < a.size() ) {
				b[center - filterSize + i] = max( c[i], d[i] );
			}
		}
	}

	return b;
}

TImage Processing( const TImage& image, const size_t filterSize )
{
	const size_t h = image.size();
	const size_t w = image[0].size();
	TImage out( h );
	for( size_t i = 0; i < h; ++i ) {
		out[i] = calculateHGW( image[i], filterSize );
	}
	return out;
}

TImage GetImage( const BitmapData& data )
{
	TImage image;
	const int bpr = data.Stride;
	const int bpp = 3; // BGR24
	BYTE* pBuffer = ( BYTE* ) data.Scan0;
	const size_t w = data.Width;
	const size_t h = data.Height;
	image.resize( h );
	int baseAdr = 0;
	for( size_t y = 0; y < h; y++ ) {
		image[y].resize( w );
		int pixelAdr = baseAdr;
		for( size_t x = 0; x < w; x++ ) {
			image[y][x] = pBuffer[pixelAdr];
			pixelAdr += bpp;
		}
		baseAdr += bpr;
	}

	return image;
}

// Для транспонированного изображения
void SetImageForTr( BitmapData& data, const TImage& image )
{
	const int bpr = data.Stride;
	const int bpp = 3; // BGR24
	BYTE* pBuffer = ( BYTE* ) data.Scan0;
	const size_t w = data.Width;
	const size_t h = data.Height;
	int baseAdr = 0;
	for( size_t y = 0; y < h; y++ ) {
		int pixelAdr = baseAdr;
		for( size_t x = 0; x < w; x++ ) {
			// обращаемся по [х] [у] потому что 
			// на вход подается транспонированное изображение
			pBuffer[pixelAdr] = image[x][y];
			pBuffer[pixelAdr + 1] = image[x][y];
			pBuffer[pixelAdr + 2] = image[x][y];
			pixelAdr += bpp;
		}
		baseAdr += bpr;
	}
}

void MorphOp( BitmapData& data, const size_t filterSize )
{
	const size_t w = data.Width;
	const size_t h = data.Height;
	wcout << "Getting image.." << endl;
	TImage image = GetImage( data );
	// 1 проход по вертикали
	wcout << "First trace..." << endl;
	TImage result = Processing( image, filterSize );
	wcout << "Transposition..." << endl;
	// Транспонируем
	TImage imageTr;
	imageTr.resize( w );
	for( size_t i = 0; i < w; ++i ) {
		for( size_t j = 0; j < h; ++j ) {
			imageTr[i].push_back( result[j][i] );
		}
	}
	wcout << "Second trace..." << endl;
	// 2 проход по горизонтали
	TImage resTr = Processing( imageTr, filterSize );
	wcout << "Setting image..." << endl;
	SetImageForTr( data, resTr );
}

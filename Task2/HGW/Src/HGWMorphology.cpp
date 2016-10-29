#include <HGWMorphology.h>
#include <cstdlib>

std::vector<BYTE> calculateHGW( const std::vector<BYTE>& a, const size_t filterSize )
{
	// todo: ��������� ������ � ����� 0�� ��� ������ �����������
	// � ������� ������� �� ���
	std::vector<BYTE> b( a.size() );
	std::vector<BYTE> c( a.size() );
	std::vector<BYTE> d( a.size() );
	int s = 2 * filterSize + 1;
	for( int center = filterSize; center < a.size(); center += s - 1 ) {
		d[0] = a[center];
		for( int i = 1; i < s - 1; ++i ) {
			if( center + i > a.size() - 1 ) {
				d[i] = max( d[i - 1], 0 );
				continue;
			}
			d[i] = max( d[i - 1], a[center + i] );
		}

		c[s - 2] = a[center - 1];
		for( int i = 1; i < s - 1; ++i ) {
			if( center - 1 - i < 0 ) {
				c[s - 2 - i] = max( c[s - 1 - i], 0 );
				continue;
			}
			c[s - 2 - i] = max( c[s - 1 - i], a[center - 1 - i] );
		}

		for( int i = 0; i < s - 1; ++i ) {
			if( center - filterSize + i > a.size() - 1 ) {
				continue;
			}
			b[center - filterSize + i] = max( c[i], d[i] );
		}
	}

	return b;
}


TImage processing( const TImage& image, const size_t filterSize )
{
	const size_t h = image.size();
	const size_t w = image[0].size();
	TImage out( h );
	for( size_t i = 0; i < h; ++i ) {
		out[i] = calculateHGW( image[i], filterSize );
	}
	return out;
}

void MorphOp( BitmapData& data, const size_t filterSize )
{
	std::vector< std::vector<BYTE> > image;
	const int bpr = data.Stride;
	const int bpp = 3; // BGR24
	BYTE* pBuffer = ( BYTE* ) data.Scan0;
	const size_t w = data.Width;
	const size_t h = data.Height;
	image.resize( h );

	// ���� ��������
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

	// 1 ������
	TImage result = processing( image, filterSize );


	// todo: save result
	/*// ����������������
	std::vector < std::vector<BYTE> > imageTr;
	imageTr.resize( w );
	for( size_t i = 0; i < w; ++i ) {
		imageTr[i].resize( h );
	}
	for( size_t i = 0; i < h; ++i ) {
		for( size_t j = 0; j < w; ++j ) {
			imageTr[j][i] = image[i][j];
		}
	}

	// 2 ������
	processing( imageTr, filterSize );*/

	baseAdr = 0;
	for( size_t y = 0; y < h; y++ ) {
		int pixelAdr = baseAdr;
		for( size_t x = 0; x < w; x++ ) {
			pBuffer[pixelAdr] = result[y][x];
			pBuffer[pixelAdr + 1] = result[y][x];
			pBuffer[pixelAdr + 2] = result[y][x];
			pixelAdr += bpp;
		}
		baseAdr += bpr;
	}
}
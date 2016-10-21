#include <HGWMorphology.h>
#include <vector>

void MorphOp( BitmapData& data )
{
	std::vector< std::vector<BYTE> > image;
	const int bpr = data.Stride;
	const int bpp = 3; // BGR24
	BYTE* pBuffer = ( BYTE* ) data.Scan0;
	const size_t w = data.Width;
	const size_t h = data.Height;
	image.resize( h );

	// Сама картинка
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

	// todo: hgw algo
}
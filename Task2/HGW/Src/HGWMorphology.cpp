#include <HGWMorphology.h>

void buildRaw( std::vector<BYTE>& raw, std::vector<BYTE>& image, const size_t filterSize,  bool straightDirection )
{
	const size_t w = image.size();
	if( !straightDirection ) {
		for( size_t i = 0; i < w / 2; ++i ) {
			std::swap( image[i], image[w - i - 1] );
		}
	}
	
	raw.resize( w + 2 * filterSize );
	for( size_t j = 0; j < filterSize; ++j ) {
		if( j % filterSize == 0 ) {
			// Начало блока длины 2R
			raw[j] = image[j + filterSize];
			raw[j + w] = image[w - filterSize + j];
		} else {
			raw[j] = max( raw[j - 1], image[j + filterSize] );
			raw[j + w] = max( raw[j + w - 1], image[w - filterSize + j] );
		}
	}
	// Дополняем общую часть
	for( size_t j = 0; j < image.size(); ++j ) {
		if( j %filterSize == 0 ) {
			raw[filterSize + j] = image[j];
		} else {
			raw[filterSize + j] = max( raw[filterSize + j - 1], image[j] );
		}
	}

	if( !straightDirection ) {
		for( size_t i = 0; i < w / 2; ++i ) {
			std::swap( image[i], image[w - i - 1] );
		}
	}
}

void processing( std::vector< std::vector<BYTE> >& image, const size_t filterSize )
{
	const size_t h = image.size();
	const size_t w = image[0].size();
	for( size_t i = 0; i < h; ++i ) {
		// Дополняем буффер справа и слева, одновременно считая максимум
		std::vector<BYTE> raw1;
		buildRaw( raw1, image[i], filterSize, true );
		std::vector<BYTE> raw2;
		buildRaw( raw2, image[i], filterSize, false );
		
		// todo: make new array
	}
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

	// 1 проход
	processing( image, filterSize );

	/*// Транспонирование
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

	// 2 проход
	processing( imageTr, filterSize );*/
}
#include <BayerPattern.h>
#include <cstdlib>

// На 2 пикселя увеличим изображения со всех сторон
const int ZeroLevel = 2;

CBayerPattern::CBayerPattern( const BitmapData& bmpData )
{
	const size_t w = bmpData.Width;
	const size_t h = bmpData.Height;
	const int bpr = bmpData.Stride;
	const int bpp = 3; // BGR24
	BYTE* pBuffer = ( BYTE* ) bmpData.Scan0;

	// Resize с учетом доп линий
	image.resize( h + 4 );
	for( size_t i = 0; i < image.size(); ++i ) {
		image[i].resize( w + 4 );
	}

	int baseAdr = 0;
	for( size_t y = 0; y < h; y++ ) {
		int pixelAdr = baseAdr;

		// Adding first two lines
		if( y == 0 ) {
			for( size_t x = 0; x < w; x++ ) {
				int B = pBuffer[pixelAdr]; // blue
				int G = pBuffer[pixelAdr + 1]; // green
				int R = pBuffer[pixelAdr + 2]; // red

				// Adding TWO first pixels
				if( x == 0 ) {
					image[0][0] = RGB( R, G, B );
					image[0][1] = RGB( R, G, B );
					image[1][0] = RGB( R, G, B );
					image[1][1] = RGB( R, G, B );
				}

				// Adding LAST TWO pixels
				if( x == w - 1 ) {
					image[0][w + 2] = RGB( R, G, B );
					image[0][w + 3] = RGB( R, G, B );
					image[1][w + 2] = RGB( R, G, B );
					image[1][w + 3] = RGB( R, G, B );
				}

				// (x, y) pixel
				image[0][x + ZeroLevel] = RGB( R, G, B );
				image[1][x + ZeroLevel] = RGB( R, G, B );

				pixelAdr += bpp;
			}
		}

		// Возвращаем на место
		pixelAdr = baseAdr;
		// Adding last two lines
		if( y == h - 1 ) {
			for( size_t x = 0; x < w; x++ ) {
				int B = pBuffer[pixelAdr]; // blue
				int G = pBuffer[pixelAdr + 1]; // green
				int R = pBuffer[pixelAdr + 2]; // red

				// Adding TWO first pixels
				if( x == 0 ) {
					image[h + 2][0] = RGB( R, G, B );
					image[h + 2][1] = RGB( R, G, B );
					image[h + 3][0] = RGB( R, G, B );
					image[h + 3][1] = RGB( R, G, B );
				}

				// Adding LAST TWO pixels
				if( x == w - 1 ) {
					image[h + 2][w + 2] = RGB( R, G, B );
					image[h + 2][w + 3] = RGB( R, G, B );
					image[h + 3][w + 2] = RGB( R, G, B );
					image[h + 3][w + 3] = RGB( R, G, B );
				}

				// (x, y) pixel
				image[h + 2][x + ZeroLevel] = RGB( R, G, B );
				image[h + 3][x + ZeroLevel] = RGB( R, G, B );

				pixelAdr += bpp;
			}
		}

		// Возвращаем на место
		pixelAdr = baseAdr;
		// Usual adding
		for( size_t x = 0; x < w; x++ ) {
			// (x, y) pixel
			int B = pBuffer[pixelAdr]; // blue
			int G = pBuffer[pixelAdr + 1]; // green
			int R = pBuffer[pixelAdr + 2]; // red

			image[ZeroLevel + y][ZeroLevel + x] = RGB( R, G, B );

			pixelAdr += bpp;
		}
		baseAdr += bpr;
	}

	height = h + 2;
	width = w + 2;
}

// Pixel Grouping алгоритм
void CBayerPattern::Process()
{
	/* Алгоритм Parttern Pixel Grouping
	Дан фильтр Байера. Для каждого пикселя известна только 1 интенсивность цвета из RGB
	1. Сначала восстановим все незнакомые зеленые цвета.
	2. Имея исхродное изображение, и весь восстановленный зеленый цвет - восстановим синий и красный
	*/
	restoreGreen();
	restoreBlueRed();
}

void CBayerPattern::GetData( BitmapData& bmpData ) const
{
	const int w = bmpData.Width;
	const int h = bmpData.Height;
	const int bpr = bmpData.Stride;
	const int bpp = 3; // BGR24
	BYTE* pBuffer = ( BYTE* ) bmpData.Scan0;

	int baseAdr = 0;
	for( int y = 0; y < h; y++ ) {
		int pixelAdr = baseAdr;
		for( int x = 0; x < w; x++ ) {
			// (x, y) pixel
			pBuffer[pixelAdr] = GetBValue( image[y + ZeroLevel][x + ZeroLevel] ); // blue
			pBuffer[pixelAdr + 1] = GetGValue( image[y + ZeroLevel][x + ZeroLevel] ); // green
			pBuffer[pixelAdr + 2] = GetRValue( image[y + ZeroLevel][x + ZeroLevel] ); // red

			pixelAdr += bpp;
		}
		baseAdr += bpr;
	}
}

bool CBayerPattern::isBlue( const size_t y, const size_t x ) const
{
	return ( GetGValue( image[y][x] ) == 0 ) && ( GetRValue( image[y][x] ) == 0 );
}

bool CBayerPattern::isRed( const size_t y, const size_t x )const
{
	return ( GetBValue( image[y][x] ) == 0 ) && ( GetGValue( image[y][x] ) == 0 );
}

bool CBayerPattern::isGreen( const size_t x, const size_t y ) const
{
	return ( GetBValue( image[y][x] ) == 0 ) && ( GetRValue( image[y][x] ) == 0 );
}

void CBayerPattern::restoreGreen()
{
	for( size_t i = ZeroLevel; i < height; ++i ) {
		for( size_t j = ZeroLevel; j < width; ++j ) {
			int deltaN = 0;
			int deltaE = 0;
			int deltaW = 0;
			int deltaS = 0;
			if( isBlue( i, j ) ) {
				// deltaN
				deltaN += abs( GetBValue( image[i][j] ) - GetBValue( image[i - 2][j] ) ) * 2 +
					abs( GetGValue( image[i - 1][j] ) - GetGValue( image[i + 1][j] ) );
				// deltaE
				deltaE += abs( GetBValue( image[i][j + 2] ) - GetBValue( image[i][j] ) ) * 2 +
					abs( GetGValue( image[i][j - 1] ) - GetGValue( image[i][j + 1] ) );
				// deltaW
				deltaW += abs( GetBValue( image[i][j - 2] ) - GetBValue( image[i][j] ) ) * 2 +
					abs( GetGValue( image[i][j - 1] ) - GetGValue( image[i][j + 1] ) );
				// deltaS
				deltaS += abs( GetBValue( image[i + 2][j] ) - GetBValue( image[i][j] ) ) * 2 +
					abs( GetGValue( image[i - 1][j] ) - GetGValue( image[i + 1][j] ) );
			}

			if( isRed( i, j ) ) {
				// deltaN
				deltaN += abs( GetRValue( image[i][j] ) - GetRValue( image[i - 2][j] ) ) * 2 +
					abs( GetGValue( image[i - 1][j] ) - GetGValue( image[i + 1][j] ) );
				// deltaE
				deltaE += abs( GetRValue( image[i][j + 2] ) - GetRValue( image[i][j] ) ) * 2 +
					abs( GetGValue( image[i][j - 1] ) - GetGValue( image[i][j + 1] ) );
				// deltaW
				deltaW += abs( GetRValue( image[i][j - 2] ) - GetRValue( image[i][j] ) ) * 2 +
					abs( GetGValue( image[i][j - 1] ) - GetGValue( image[i][j + 1] ) );
				// deltaS
				deltaS += abs( GetRValue( image[i + 2][j] ) - GetRValue( image[i][j] ) ) * 2 +
					abs( GetGValue( image[i - 1][j] ) - GetGValue( image[i + 1][j] ) );
			}

			int smallestGrad = min( deltaN, min( deltaE, min( deltaW, deltaS ) ) );
			if( smallestGrad == deltaN ) {
				// todo
			}
		}
	}
}


void CBayerPattern::restoreBlueRed()
{
}
#include <BayerPattern.h>
#include <cstdlib>

// На 2 пикселя увеличим изображения со всех сторон
const int ZeroLevel = 2;

// Боремся с переполнением
BYTE toBYTE( int x )
{
	if( x > 255 ) return 255;
	if( x < 0 ) return 0;
	return ( BYTE ) x;
}

// Делаем RGB, с проверкой переполнения сигнала в канале
#define SAFE_RGB(R, G, B) RGB(toBYTE(R), toBYTE(G), toBYTE(B))
// Если значение по умолчанию для рамки не задано, то боковые строчки будут экстраполированы на 2 пикселя во все стороны,
// если задано - то def - значение этих пикселей
#define FRAME_VALUE(R, G, B, def) def == -1 ? SAFE_RGB(R, G, B) : def;

CBayerPattern::CBayerPattern( const BitmapData& bmpData, int defFrameValue )
{
	const size_t w = bmpData.Width;
	const size_t h = bmpData.Height;
	height = h;
	width = w;
	const int bpr = bmpData.Stride;
	const int bpp = 3; // BGR24
	BYTE* pBuffer = ( BYTE* ) bmpData.Scan0;

	// Resize с учетом доп линий
	image.resize( h + 4 );
	for( size_t i = 0; i < image.size(); ++i ) {
		image[i].resize( w + 4 );
	}

	// Сама картинка
	int baseAdr = 0;
	for( size_t y = 0; y < h; y++ ) {
		int pixelAdr = baseAdr;
		for( size_t x = 0; x < w; x++ ) {
			int B = pBuffer[pixelAdr]; // blue
			int G = pBuffer[pixelAdr + 1]; // green
			int R = pBuffer[pixelAdr + 2]; // red

			image[ZeroLevel + y][ZeroLevel + x] = SAFE_RGB( R, G, B );

			pixelAdr += bpp;
		}
		baseAdr += bpr;
	}

	// Экстраполируем
	// Заполним значения сверху и снизу
	fillUpperAndLowerEdges( defFrameValue );
	// Заполним значения по бокам
	fillLeftRightEdges( defFrameValue );
}

// Pattern Pixel Grouping алгоритм
void CBayerPattern::Process()
{
	/* Алгоритм Pattern Pixel Grouping
	Дан фильтр Байера. Для каждого пикселя известна только 1 интенсивность цвета из RGB
	1. Сначала восстановим все незнакомые зеленые цвета.
	2. Имея исхродное изображение, и весь восстановленный зеленый цвет - восстановим синий и красный
	Замечание: используется всего 1 буффер, так как не должно быть зависимостей при восстановлении цвета.
	*/
	wcout << L"Now restoring green..." << endl;
	restoreGreen();
	wcout << L"Now restoring blue and red..." << endl;
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

bool CBayerPattern::isBlueOnly( const size_t x, const size_t y ) const
{
	return ( ( x % 2 != 0 ) && ( y % 2 != 0 ) );
}

bool CBayerPattern::isRedOnly( const size_t x, const size_t y ) const
{
	return ( ( x % 2 == 0 ) && ( y % 2 == 0 ) );
}

bool CBayerPattern::isGreenOnly( const size_t x, const size_t y ) const
{
	return !isBlueOnly( x, y ) && !isRedOnly( x, y );
}

void CBayerPattern::restoreGreen()
{
	wcout << L"    Now restoring green on red" << endl;
	restoreGreenOnRed();
	wcout << L"    Now restoring green on blue" << endl;
	restoreGreenOnBlue();
}

void CBayerPattern::restoreGreenOnBlue()
{
	for( size_t i = ZeroLevel; i < ZeroLevel + height; ++i ) {
		for( size_t j = ZeroLevel; j < ZeroLevel + width; ++j ) {
			if( isBlueOnly( j, i ) ) {
				int r = 0;
				int g = 0;
				int b = 0;
				int deltaN = abs( GetBValue( image[i][j] ) - GetBValue( image[i - 2][j] ) ) * 2 +
					abs( GetGValue( image[i - 1][j] ) - GetGValue( image[i + 1][j] ) );
				int deltaE = abs( GetBValue( image[i][j] ) - GetBValue( image[i][j + 2] ) ) * 2 +
					abs( GetGValue( image[i][j - 1] ) - GetGValue( image[i][j + 1] ) );
				int deltaW = abs( GetBValue( image[i][j] ) - GetBValue( image[i][j - 2] ) ) * 2 +
					abs( GetGValue( image[i][j - 1] ) - GetGValue( image[i][j + 1] ) );
				int deltaS = abs( GetBValue( image[i][j] ) - GetBValue( image[i + 2][j] ) ) * 2 +
					abs( GetGValue( image[i - 1][j] ) - GetGValue( image[i + 1][j] ) );
				int smallestGrad = min( deltaN, min( deltaE, min( deltaW, deltaS ) ) );
				if( smallestGrad == deltaN ) {
					r = GetRValue( image[i][j] );
					g = ( GetGValue( image[i - 1][j] ) * 3 + GetGValue( image[i + 1][j] ) + GetBValue( image[i][j] ) - GetBValue( image[i - 2][j] ) ) / 4;
					b = GetBValue( image[i][j] );
				} else if( smallestGrad == deltaE ) {
					r = GetRValue( image[i][j] );
					g = ( GetGValue( image[i][j + 1] ) * 3 + GetGValue( image[i][j - 1] ) + GetBValue( image[i][j] ) - GetBValue( image[i][j + 2] ) ) / 4;
					b = GetBValue( image[i][j] );
				} else if( smallestGrad == deltaW ) {
					r = GetRValue( image[i][j] );
					g = ( GetGValue( image[i][j - 1] ) * 3 + GetGValue( image[i][j + 1] ) + GetBValue( image[i][j] ) - GetBValue( image[i][j - 2] ) ) / 4;
					b = GetBValue( image[i][j] );
				} else if( smallestGrad == deltaS ) {
					r = GetRValue( image[i][j] );
					g = ( GetGValue( image[i + 1][j] ) * 3 + GetGValue( image[i - 1][j] ) + GetBValue( image[i][j] ) - GetBValue( image[i + 2][j] ) ) / 4;
					b = GetBValue( image[i][j] );
				}
				image[i][j] = SAFE_RGB( r, g, b );
			}
		}
	}
}

void CBayerPattern::restoreGreenOnRed()
{
	for( size_t i = ZeroLevel; i < ZeroLevel + height; ++i ) {
		for( size_t j = ZeroLevel; j < ZeroLevel + width; ++j ) {
			if( isRedOnly( j, i ) ) {
				int r = 0;
				int g = 0;
				int b = 0;
				int deltaN = abs( GetRValue( image[i][j] ) - GetRValue( image[i - 2][j] ) ) * 2 +
					abs( GetGValue( image[i - 1][j] ) - GetGValue( image[i + 1][j] ) );
				int deltaE = abs( GetRValue( image[i][j] ) - GetRValue( image[i][j + 2] ) ) * 2 +
					abs( GetGValue( image[i][j - 1] ) - GetGValue( image[i][j + 1] ) );
				int deltaW = abs( GetRValue( image[i][j] ) - GetRValue( image[i][j - 2] ) ) * 2 +
					abs( GetGValue( image[i][j - 1] ) - GetGValue( image[i][j + 1] ) );
				int deltaS = abs( GetRValue( image[i][j] ) - GetRValue( image[i + 2][j] ) ) * 2 +
					abs( GetGValue( image[i - 1][j] ) - GetGValue( image[i + 1][j] ) );
				int smallestGrad = min( deltaN, min( deltaE, min( deltaW, deltaS ) ) );
				if( smallestGrad == deltaN ) {
					r = GetRValue( image[i][j] );
					g = ( GetGValue( image[i - 1][j] ) * 3 + GetGValue( image[i + 1][j] ) + GetRValue( image[i][j] ) - GetRValue( image[i - 2][j] ) ) / 4;
					b = GetBValue( image[i][j] );
				} else if( smallestGrad == deltaE ) {
					r = GetRValue( image[i][j] );
					g = ( GetGValue( image[i][j + 1] ) * 3 + GetGValue( image[i][j - 1] ) + GetRValue( image[i][j] ) - GetRValue( image[i][j + 2] ) ) / 4;
					b = GetBValue( image[i][j] );
				} else if( smallestGrad == deltaW ) {
					r = GetRValue( image[i][j] );
					g = ( GetGValue( image[i][j - 1] ) * 3 + GetGValue( image[i][j + 1] ) + GetRValue( image[i][j] ) - GetRValue( image[i][j - 2] ) ) / 4;
					b = GetBValue( image[i][j] );
				} else if( smallestGrad == deltaS ) {
					r = GetRValue( image[i][j] );
					g = ( GetGValue( image[i + 1][j] ) * 3 + GetGValue( image[i - 1][j] ) + GetRValue( image[i][j] ) - GetRValue( image[i + 2][j] ) ) / 4;
					b = GetBValue( image[i][j] );
				}
				image[i][j] = SAFE_RGB( r, g, b );
			}
		}
	}
}

void CBayerPattern::restoreBlueRed()
{
	wcout << "    Now restoring red and blue values at green" << endl;
	computeRedBlueAtGreen();
	wcout << "    Now restoring blue values at red" << endl;
	computeBlueAtRed();
	wcout << "    Now restoring red values at blue" << endl;
	computeRedAtBlue();
}

void CBayerPattern::computeRedBlueAtGreen()
{
	for( size_t i = ZeroLevel; i < ZeroLevel + height; ++i ) {
		for( size_t j = ZeroLevel; j < ZeroLevel + width; ++j ) {
			if( isGreenOnly( j, i ) ) {
				int r = 0;
				int b = 0;
				if( ( i % 2 != 0 ) && ( j % 2 == 0 ) ) {
					r = hueTransit( GetGValue( image[i - 1][j] ), GetGValue( image[i][j] ), GetGValue( image[i + 1][j] ), GetRValue( image[i - 1][j] ), GetRValue( image[i + 1][j] ) );
					b = hueTransit( GetGValue( image[i][j - 1] ), GetGValue( image[i][j] ), GetGValue( image[i][j + 1] ), GetBValue( image[i][j - 1] ), GetBValue( image[i][j + 1] ) );
				} else {
					b = hueTransit( GetGValue( image[i - 1][j] ), GetGValue( image[i][j] ), GetGValue( image[i + 1][j] ), GetBValue( image[i - 1][j] ), GetBValue( image[i + 1][j] ) );
					r = hueTransit( GetGValue( image[i][j - 1] ), GetGValue( image[i][j] ), GetGValue( image[i][j + 1] ), GetRValue( image[i][j - 1] ), GetRValue( image[i][j + 1] ) );
				}
				image[i][j] = SAFE_RGB( r, GetGValue( image[i][j] ), b );
			}
		}
	}
}

void CBayerPattern::computeBlueAtRed()
{
	for( size_t i = ZeroLevel; i < ZeroLevel + height; ++i ) {
		for( size_t j = ZeroLevel; j < ZeroLevel + width; ++j ) {
			if( isRedOnly( j, i ) ) {
				int b = 0;
				int deltaNE = abs( GetBValue( image[i - 1][j + 1] ) - GetBValue( image[i + 1][j - 1] ) ) +
					abs( GetRValue( image[i - 2][j + 2] ) - GetRValue( image[i][j] ) ) +
					abs( GetRValue( image[i][j] ) - GetRValue( image[i + 2][j - 2] ) ) +
					abs( GetGValue( image[i - 1][j + 1] ) - GetGValue( image[i][j] ) ) +
					abs( GetGValue( image[i][j] ) - GetGValue( image[i + 1][j - 1] ) );
				int deltaNW = abs( GetBValue( image[i - 1][j - 1] ) - GetBValue( image[i + 1][j + 1] ) ) +
					abs( GetRValue( image[i - 2][j - 2] ) - GetRValue( image[i][j] ) ) +
					abs( GetRValue( image[i][j] ) - GetRValue( image[i + 2][j + 2] ) ) +
					abs( GetGValue( image[i - 1][j - 1] ) - GetGValue( image[i][j] ) ) +
					abs( GetGValue( image[i][j] ) - GetGValue( image[i + 1][j + 1] ) );
				if( deltaNE < deltaNW ) {
					b = hueTransit( GetGValue( image[i - 1][j + 1] ), GetGValue( image[i][j] ), GetGValue( image[i + 1][j - 1] ), GetBValue( image[i - 1][j + 1] ), GetBValue( image[i + 1][j - 1] ) );
				} else {
					b = hueTransit( GetGValue( image[i - 1][j - 1] ), GetGValue( image[i][j] ), GetGValue( image[i + 1][j + 1] ), GetBValue( image[i - 1][j - 1] ), GetBValue( image[i + 1][j + 1] ) );
				}
				image[i][j] = SAFE_RGB( GetRValue( image[i][j] ), GetGValue( image[i][j] ), b );
			}
		}
	}
}

void CBayerPattern::computeRedAtBlue()
{
	for( size_t i = ZeroLevel; i < ZeroLevel + height; ++i ) {
		for( size_t j = ZeroLevel; j < ZeroLevel + width; ++j ) {
			if( isBlueOnly( j, i ) ) {
				int r = 0;
				int deltaNE = abs( GetRValue( image[i - 1][j + 1] ) - GetRValue( image[i + 1][j - 1] ) ) +
					abs( GetBValue( image[i - 2][j + 2] ) - GetBValue( image[i][j] ) ) +
					abs( GetBValue( image[i][j] ) - GetBValue( image[i + 2][j - 2] ) ) +
					abs( GetGValue( image[i - 1][j + 1] ) - GetGValue( image[i][j] ) ) +
					abs( GetGValue( image[i][j] ) - GetGValue( image[i + 1][j - 1] ) );
				int deltaNW = abs( GetRValue( image[i - 1][j - 1] ) - GetRValue( image[i + 1][j + 1] ) ) +
					abs( GetBValue( image[i - 2][j - 2] ) - GetBValue( image[i][j] ) ) +
					abs( GetBValue( image[i][j] ) - GetBValue( image[i + 2][j + 2] ) ) +
					abs( GetGValue( image[i - 1][j - 1] ) - GetGValue( image[i][j] ) ) +
					abs( GetGValue( image[i][j] ) - GetGValue( image[i + 1][j + 1] ) );
				if( deltaNE < deltaNW ) {
					r = hueTransit( GetGValue( image[i - 1][j + 1] ), GetGValue( image[i][j] ), GetGValue( image[i + 1][j - 1] ), GetRValue( image[i - 1][j + 1] ), GetRValue( image[i + 1][j - 1] ) );
				} else {
					r = hueTransit( GetGValue( image[i - 1][j - 1] ), GetGValue( image[i][j] ), GetGValue( image[i + 1][j + 1] ), GetRValue( image[i - 1][j - 1] ), GetRValue( image[i + 1][j + 1] ) );
				}
				image[i][j] = SAFE_RGB( r, GetGValue( image[i][j] ), GetBValue( image[i][j] ) );
			}
		}
	}
}

int CBayerPattern::hueTransit( int l1, int l2, int l3, int v1, int v3 ) const
{
	if( ( ( l1 < l2 ) && ( l2 < l3 ) ) || ( ( l1 > l2 ) && ( l2 > l3 ) ) ) {
		return v1 + ( v3 - v1 ) * ( l2 - l1 ) / ( l3 - l1 );
	} else {
		return ( v1 + v3 ) / 2 + ( l2 * 2 - l1 - l3 ) / 4;
	}
}

// Заполним значения по бокам
void CBayerPattern::fillLeftRightEdges( int defFrameValue )
{
	for( size_t i = 0; i < height + 4; ++i ) {
		image[i][0] = FRAME_VALUE(
			GetRValue( image[i][ZeroLevel] ),
			GetGValue( image[i][ZeroLevel] ),
			GetBValue( image[i][ZeroLevel] ),
			defFrameValue );
		image[i][1] = FRAME_VALUE(
			GetRValue( image[i][ZeroLevel + 1] ),
			GetGValue( image[i][ZeroLevel + 1] ),
			GetBValue( image[i][ZeroLevel + 1] ),
			defFrameValue );
		image[i][width + 3] = FRAME_VALUE(
			GetRValue( image[i][width + 3 - ZeroLevel] ),
			GetGValue( image[i][width + 3 - ZeroLevel] ),
			GetBValue( image[i][width + 3 - ZeroLevel] ),
			defFrameValue );
		image[i][width + 2] = FRAME_VALUE(
			GetRValue( image[i][width + 2 - ZeroLevel] ),
			GetGValue( image[i][width + 2 - ZeroLevel] ),
			GetBValue( image[i][width + 2 - ZeroLevel] ),
			defFrameValue );
	}
}

// Заполним значения сверху и снизу
void CBayerPattern::fillUpperAndLowerEdges( int defFrameValue )
{
	for( size_t x = ZeroLevel; x < ZeroLevel + width; ++x ) {
		image[0][x] = FRAME_VALUE(
			GetRValue( image[ZeroLevel][x] ),
			GetGValue( image[ZeroLevel][x] ),
			GetBValue( image[ZeroLevel][x] ),
			defFrameValue );
		image[1][x] = FRAME_VALUE(
			GetRValue( image[ZeroLevel + 1][x] ),
			GetGValue( image[ZeroLevel + 1][x] ),
			GetBValue( image[ZeroLevel + 1][x] ),
			defFrameValue );
		image[height + 2][x] = FRAME_VALUE(
			GetRValue( image[ZeroLevel + height - 2][x] ),
			GetGValue( image[ZeroLevel + height - 2][x] ),
			GetBValue( image[ZeroLevel + height - 2][x] ),
			defFrameValue );
		image[height + 3][x] = FRAME_VALUE(
			GetRValue( image[ZeroLevel + height - 1][x] ),
			GetGValue( image[ZeroLevel + height - 1][x] ),
			GetBValue( image[ZeroLevel + height - 1][x] ),
			defFrameValue );
	}
}

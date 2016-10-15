#include <BayerPattern.h>
#include <cstdlib>
#include <cassert>
#include <algorithm>

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

// Pixel Grouping алгоритм
void CBayerPattern::Process()
{
	/* Алгоритм Parttern Pixel Grouping
	Дан фильтр Байера. Для каждого пикселя известна только 1 интенсивность цвета из RGB
	1. Сначала восстановим все незнакомые зеленые цвета.
	2. Имея исхродное изображение, и весь восстановленный зеленый цвет - восстановим синий и красный
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

std::vector<int> CBayerPattern::GetGist() const
{
	const int LumaRed = 9798;
	const int LumaGreen = 19235;
	const int LumaBlue = 3735;
	const int CoeffNormalizationBitsCount = 15;
	const int CoeffNormalization = 1 << CoeffNormalizationBitsCount;
	std::vector<int> hist( 256, 0 );
	for( size_t i = ZeroLevel; i < ZeroLevel + height; ++i ) {
		for( size_t j = ZeroLevel; j < ZeroLevel + width; ++j ) {
			int r = GetRValue( image[i][j] );
			int g = GetGValue( image[i][j] );
			int b = GetBValue( image[i][j] );
			int y = ( LumaRed * r + LumaGreen * g + LumaBlue * b + ( CoeffNormalization >> 1 ) ) >> CoeffNormalizationBitsCount;
			++hist[y];
		}
	}
	return hist;
}

bool CBayerPattern::isBlueOnly( const size_t x, const size_t y ) const
{
	return ( GetGValue( image[y][x] ) == 0 ) && ( GetRValue( image[y][x] ) == 0 );
}

bool CBayerPattern::isRedOnly( const size_t x, const size_t y ) const
{
	return ( GetBValue( image[y][x] ) == 0 ) && ( GetGValue( image[y][x] ) == 0 );
}

bool CBayerPattern::isGreenOnly( const size_t x, const size_t y ) const
{
	return ( GetBValue( image[y][x] ) == 0 ) && ( GetRValue( image[y][x] ) == 0 );
}

// ????????????????????????????
void CBayerPattern::restoreGreen()
{
	for( size_t i = ZeroLevel; i < ZeroLevel + height; ++i ) {
		for( size_t j = ZeroLevel; j < ZeroLevel + width; ++j ) {
			if( isGreenOnly( j, i ) ) {
				// Восстанавливаем только на голубых и красных рецепторах
				continue;
			}
			int deltaN = 0;
			int deltaE = 0;
			int deltaW = 0;
			int deltaS = 0;
			if( isRedOnly( j, i ) ) {          //   0 1 2 3
				
				//1383 1340
				// 1365 1304
				// deltaN
				deltaN = abs( GetRValue( image[i][j] ) - GetRValue( image[i - 2][j] ) ) * 2 +
					abs( GetGValue( image[i - 1][j] ) - GetGValue( image[i + 1][j] ) );
				// deltaE
				deltaE = abs( GetRValue( image[i][j] ) - GetRValue( image[i][j + 2] ) ) * 2 +
					abs( GetGValue( image[i][j - 1] ) - GetGValue( image[i][j + 1] ) );
				// deltaW
				deltaW = abs( GetRValue( image[i][j] ) - GetRValue( image[i][j - 2] ) ) * 2 +
					abs( GetGValue( image[i][j - 1] ) - GetGValue( image[i][j + 1] ) );
				// deltaS
				deltaS = abs( GetRValue( image[i][j] ) - GetRValue( image[i + 2][j] ) ) * 2 +
					abs( GetGValue( image[i - 1][j] ) - GetGValue( image[i + 1][j] ) );
				// !!!!!!!!!!!!!!!!!!!! TODO
				int smallestGrad = min( deltaN, min( deltaE, min( deltaW, deltaS ) ) );
				if( smallestGrad == deltaN ) {
					image[i][j] = SAFE_RGB( 
						GetRValue( image[i][j] ),
						( GetGValue( image[i - 1][j] ) * 3 + GetGValue( image[i + 1][j] ) + GetRValue( image[i][j] ) - GetRValue( image[i - 2][j] ) ) / 4,
						GetBValue( image[i][j] ) );
				} else if( smallestGrad == deltaE ) {
					image[i][j] = SAFE_RGB(
						GetRValue( image[i][j] ),
						( GetGValue( image[i][j + 1] ) * 3 + GetGValue( image[i][j - 1] ) + GetRValue( image[i][j] ) - GetRValue( image[i][j + 2] ) ) / 4,
						GetBValue( image[i][j] ) );
				} else if( smallestGrad == deltaW ) {
					image[i][j] = SAFE_RGB( 
						GetRValue( image[i][j] ),
						( GetGValue( image[i][j - 1] ) * 3 + GetGValue( image[i][j + 1] ) + GetRValue( image[i][j] ) - GetRValue( image[i][j - 2] ) ) / 4,
						GetBValue( image[i][j] ) );
				} else if( smallestGrad == deltaS ) {
					image[i][j] = SAFE_RGB( 
						GetRValue( image[i][j] ),
						( GetGValue( image[i + 1][j] ) * 3 + GetGValue( image[i - 1][j] ) + GetRValue( image[i][j] ) - GetRValue( image[i + 2][j] ) ) / 4,
						GetBValue( image[i][j] ) );
				} else {
					assert( false );
				}
			} else if( isBlueOnly( j, i ) ) {
				// deltaN
				deltaN = abs( GetBValue( image[i][j] ) - GetBValue( image[i - 2][j] ) ) * 2 +
					abs( GetGValue( image[i - 1][j] ) - GetGValue( image[i + 1][j] ) );
				// deltaE
				deltaE = abs( GetBValue( image[i][j] ) - GetBValue( image[i][j + 2] ) ) * 2 +
					abs( GetGValue( image[i][j - 1] ) - GetGValue( image[i][j + 1] ) );
				// deltaW
				deltaW = abs( GetBValue( image[i][j] ) - GetBValue( image[i][j - 2] ) ) * 2 +
					abs( GetGValue( image[i][j - 1] ) - GetGValue( image[i][j + 1] ) );
				// deltaS
				deltaS = abs( GetBValue( image[i][j] ) - GetBValue( image[i + 2][j] ) ) * 2 +
					abs( GetGValue( image[i - 1][j] ) - GetGValue( image[i + 1][j] ) );
				int smallestGrad = min( deltaN, min( deltaE, min( deltaW, deltaS ) ) );
				if( smallestGrad == deltaN ) {
					image[i][j] = SAFE_RGB( GetRValue( image[i][j] ),
						( GetGValue( image[i - 1][j] ) * 3 + GetGValue( image[i + 1][j] ) + GetBValue( image[i][j] ) - GetBValue( image[i - 2][j] ) ) / 4,
						GetBValue( image[i][j] ) );
				} else if( smallestGrad == deltaE ) {
					image[i][j] = SAFE_RGB( GetRValue( image[i][j] ),
						( GetGValue( image[i][j + 1] ) * 3 + GetGValue( image[i][j - 1] ) + GetBValue( image[i][j] ) - GetBValue( image[i][j + 2] ) ) / 4,
						GetBValue( image[i][j] ) );
				} else if( smallestGrad == deltaW ) {
					image[i][j] = SAFE_RGB( GetRValue( image[i][j] ),
						( GetGValue( image[i][j - 1] ) * 3 + GetGValue( image[i][j + 1] ) + GetBValue( image[i][j] ) - GetBValue( image[i][j - 2] ) ) / 4,
						GetBValue( image[i][j] ) );
				} else if( smallestGrad == deltaS ) {
					image[i][j] = SAFE_RGB( GetRValue( image[i][j] ),
						( GetGValue( image[i + 1][j] ) * 3 + GetGValue( image[i - 1][j] ) + GetBValue( image[i][j] ) - GetBValue( image[i + 2][j] ) ) / 4,
						GetBValue( image[i][j] ) );
				} else {
					assert( false );
				}
			} else {
				assert( false );
			}
		}
	}
}

void CBayerPattern::restoreBlueRed()
{
	wcout << "    Now will compute red and blue values at green" << endl;
	// Восстанавливаем в ячейках, где есть только G
	computeRedBlueAtGreen();
	wcout << "    Now will compute blue values at red" << endl;
	// Восстанавливаем в ячейках, где есть R, G
	computeBlueAtRed();
	wcout << "    Now will compute red values at blue" << endl;
	// Восстанавливаем в ячейках, где есть G, B
	computeRedAtBlue();
}

// +++++++++++++++++++++++++++++++++++++++
void CBayerPattern::computeRedBlueAtGreen()
{
	for( size_t i = ZeroLevel; i < ZeroLevel + height; ++i ) {
		for( size_t j = ZeroLevel; j < ZeroLevel + width; ++j ) {
			if( isGreenOnly( j, i ) ) {
				if( j == 1389 && i == 1036 ) { // 0 R G  R G
					DebugBreak();              // 1 G B  G B
				}
				int r = hueTransit( GetGValue( image[i - 1][j] ), GetGValue( image[i][j] ), GetGValue( image[i + 1][j] ), GetRValue( image[i - 1][j] ), GetRValue( image[i + 1][j] ) );
				int b = hueTransit( GetGValue( image[i][j - 1] ), GetGValue( image[i][j] ), GetGValue( image[i][j + 1] ), GetBValue( image[i][j - 1] ), GetBValue( image[i][j + 1] ) );
				image[i][j] = SAFE_RGB( 
					r,
					GetGValue( image[i][j] ),
					b );
			}
		}
	}
}

void CBayerPattern::computeBlueAtRed()
{
	for( size_t i = ZeroLevel; i < ZeroLevel + height; ++i ) {
		for( size_t j = ZeroLevel; j < ZeroLevel + width; ++j ) {
			if( GetBValue( image[i][j] ) == 0 ) {
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
				int smallestGrad = min( deltaNE, deltaNW );
				if( smallestGrad == deltaNE ) {
					image[i][j] = SAFE_RGB( GetRValue( image[i][j] ),
						GetGValue( image[i][j] ),
						hueTransit( GetGValue( image[i - 1][j + 1] ), GetGValue( image[i][j] ), GetGValue( image[i + 1][j - 1] ), GetBValue( image[i - 1][j + 1] ), GetBValue( image[i + 1][j - 1] ) ) );
				} else if( smallestGrad == deltaNW ) {
					image[i][j] = SAFE_RGB( GetRValue( image[i][j] ),
						GetGValue( image[i][j] ),
						hueTransit( GetGValue( image[i - 1][j - 1] ), GetGValue( image[i][j] ), GetGValue( image[i + 1][j + 1] ), GetBValue( image[i - 1][j - 1] ), GetBValue( image[i + 1][j + 1] ) ) );
				}
			}
		}
	}
}

void CBayerPattern::computeRedAtBlue()
{
	for( size_t i = ZeroLevel; i < ZeroLevel + height; ++i ) {
		for( size_t j = ZeroLevel; j < ZeroLevel + width; ++j ) {
			if( GetRValue( image[i][j] ) == 0 ) {
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
				int smallestGrad = min( deltaNE, deltaNW );
				if( smallestGrad == deltaNE ) {
					image[i][j] = SAFE_RGB( hueTransit( GetGValue( image[i - 1][j + 1] ), GetGValue( image[i][j] ), GetGValue( image[i + 1][j - 1] ), GetRValue( image[i - 1][j + 1] ), GetRValue( image[i + 1][j - 1] ) ),
						GetGValue( image[i][j] ),
						GetBValue( image[i][j] ) );
				} else if( smallestGrad == deltaNW ) {
					image[i][j] = SAFE_RGB( hueTransit( GetGValue( image[i - 1][j - 1] ), GetGValue( image[i][j] ), GetGValue( image[i + 1][j + 1] ), GetRValue( image[i - 1][j - 1] ), GetRValue( image[i + 1][j + 1] ) ),
						GetGValue( image[i][j] ),
						GetBValue( image[i][j] ) );
				}
			}
		}
	}
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++
int CBayerPattern::hueTransit( int l1, int l2, int l3, int v1, int v3 ) const
{
	if( ( ( l1 < l2 ) && ( l2 < l3 ) ) || ( ( l1 > l2 ) && ( l2 > l3 ) ) ) {
		return v1 + ( v3 - v1 ) * ( l2 - l1 ) / ( l3 - l1 );
	} else {
		return ( v1 + v3 ) / 2 + ( l2 * 2 - l1 - l3 ) / 4;
	}
}

void CBayerPattern::gammaCorrection( double gama )
{
	for( size_t i = 0; i < 4 + height; ++i ) {
		for( size_t j = 0; j < 4 + width; ++j ) {
			image[i][j] = SAFE_RGB( ( int ) pow( GetRValue( image[i][j] ) / 255.0, gama ) * 255,
				( int ) pow( GetGValue( image[i][j] ) / 255.0, gama ) * 255,
				( int ) pow( GetBValue( image[i][j] ) / 255.0, gama ) * 255 );
		}
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

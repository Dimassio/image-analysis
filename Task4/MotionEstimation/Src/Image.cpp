#include <Common.h>
#pragma hdrstop

#include <Image.h>

CImage::CImage( const BitmapData& bmpData )
{
	initializeNodesToCheck();

	const int bpr = bmpData.Stride;
	const int bpp = 3; // BGR24
	BYTE* pBuffer = ( BYTE* ) bmpData.Scan0;
	width = bmpData.Width;
	height = bmpData.Height;

	image.resize( height );
	int baseAdr = 0;
	for( int y = 0; y < height; y++ ) {
		image[y].resize( width );
		int pixelAdr = baseAdr;
		for( int x = 0; x < width; x++ ) {
			BYTE B = pBuffer[pixelAdr]; // blue
			BYTE G = pBuffer[pixelAdr + 1]; // green
			BYTE R = pBuffer[pixelAdr + 2]; // red
			image[y][x] = static_cast< BYTE >( 0.2125 * R + 0.7154 * G + 0.0721 * B );
			pixelAdr += bpp;
		}
		baseAdr += bpr;
	}

	// Оставляем рамку по бокам в radius клеток
	for( int i = radius; i < height - radius; i += radius * 2 + 1 ) {
		for( int j = radius; j < width - radius; j += radius * 2 + 1 ) {
			blocks.push_back( CPoint( j, i ) );
		}
	}
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
			pBuffer[pixelAdr] = image[y][x];
			pBuffer[pixelAdr + 1] = image[y][x];
			pBuffer[pixelAdr + 2] = image[y][x];
			pixelAdr += bpp;
		}
		baseAdr += bpr;
	}
}

CVector CImage::EstimateMotionVectorFrom( const CImage& other )
{
	CVector averageVector( 0, 0 );
	for( size_t i = 0; i < blocks.size(); ++i ) {
		CVector currVector = getBlockVector( blocks[i], other.image );
		averageVector += currVector;
	}
	double x = static_cast< double >( averageVector.x ) / static_cast< int >( blocks.size() );
	double y = static_cast< double >( averageVector.y ) / static_cast< int >( blocks.size() );
	// todo: нужно по полученным векторам определить глобальный вектор
	wcout << x << " " << -y << endl;
	return CVector( x, y );
}

void CImage::initializeNodesToCheck()
{
	nodesToCheck.resize( 9 );
	nodesToCheck[0] = { 0, 1, 2, 3, 4, 5, 6, 7, 8 };
	nodesToCheck[1] = { 1, 2, 3, 7, 8 };
	nodesToCheck[2] = { 1, 2, 3 };
	nodesToCheck[3] = { 1, 2, 3, 4, 5 };
	nodesToCheck[4] = { 3, 4, 5 };
	nodesToCheck[5] = { 3, 4, 5, 6, 7 };
	nodesToCheck[6] = { 5, 6, 7 };
	nodesToCheck[7] = { 1, 5, 6, 7, 8 };
	nodesToCheck[8] = { 1, 7, 8 };
}

int CImage::getPointNumber( const CPoint& center, const CPoint& currPoint ) const
{
	switch( currentSP ) {
		case SP_LDSP:
		{
			CVector delta = currPoint - center;
			if( delta == CVector( 0, 0 ) ) {
				return 0;
			}
			if( delta == CVector( 0, -2 ) ) {
				return 1;
			}
			if( delta == CVector( 1, -1 ) ) {
				return 2;
			}
			if( delta == CVector( 2, 0 ) ) {
				return 3;
			}
			if( delta == CVector( 1, 1 ) ) {
				return 4;
			}
			if( delta == CVector( 0, 2 ) ) {
				return 5;
			}
			if( delta == CVector( -1, 1 ) ) {
				return 6;
			}
			if( delta == CVector( -2, 0 ) ) {
				return 7;
			}
			if( delta == CVector( -1, -1 ) ) {
				return 8;
			}
			// unknown point
			assert( false );
			return -1;
		}
		case SP_SDSP:
		{
			CVector delta = currPoint - center;
			if( delta == CVector( 0, 0 ) ) {
				return 0;
			}
			if( delta == CVector( 0, -1 ) ) {
				return 1;
			}
			if( delta == CVector( 1, 0 ) ) {
				return 2;
			}
			if( delta == CVector( 0, 1 ) ) {
				return 3;
			}
			if( delta == CVector( -1, 0 ) ) {
				return 4;
			}
			// unknown point
			assert( false );
			return -1;
		}
		default:
			// unknown shape
			assert( false );
			return -1;
	}
}

CPoint CImage::getNodePoint( const CPoint& center, int number ) const
{
	CPoint node;
	switch( currentSP ) {
		case SP_LDSP:
		{
			switch( number ) {
				case 0:
					node = center + CVector( 0, 0 );
					break;
				case 1:
					node = center + CVector( 0, -2 );
					break;
				case 2:
					node = center + CVector( 1, -1 );
					break;
				case 3:
					node = center + CVector( 2, 0 );
					break;
				case 4:
					node = center + CVector( 1, 1 );
					break;
				case 5:
					node = center + CVector( 0, 2 );
					break;
				case 6:
					node = center + CVector( -1, 1 );
					break;
				case 7:
					node = center + CVector( -2, 0 );
					break;
				case 8:
					node = center + CVector( -1, -1 );
					break;
				default:
					assert( false );
			}
			break;
		}
		case SP_SDSP:
		{
			switch( number ) {
				case 0:
					node = center + CVector( 0, 0 );
					break;
				case 1:
					node = center + CVector( 0, -1 );
					break;
				case 2:
					node = center + CVector( 1, 0 );
					break;
				case 3:
					node = center + CVector( 0, 1 );
					break;
				case 4:
					node = center + CVector( -1, 0 );
					break;
				default:
					assert( false );
			}
			break;
		}
		default:
			// unknown shape
			assert( false );
			return -1;
	}
	return node;
}

std::vector<CPoint> CImage::getPointsToCheck( const CPoint& center, const CPoint& currPoint ) const
{
	std::vector<CPoint> points;
	std::vector<int> neighbours;
	if( currentSP == SP_SDSP ) {
		neighbours = { 0, 1, 2, 3, 4 };
	} else {
		neighbours = nodesToCheck[getPointNumber( center, currPoint )];
	}
	for( size_t i = 0; i < neighbours.size(); ++i ) {
		CPoint nodePoint = getNodePoint( currPoint, neighbours[i] );
		if( inTheBox( nodePoint ) ) {
			points.push_back( nodePoint );
		}
	}

	return points;
}

CVector CImage::getBlockVector( const CPoint& block, const TImage& otherImg )
{
	CPoint startPoint = block;
	currentCenter = startPoint;
	currentSP = SP_LDSP;
	error = INT_MAX;
	minimumPoint = CPoint( 0, 0 );
	CPoint resultPoint = getFinalPoint( block, block, otherImg );
	return resultPoint - startPoint;
}

// Center - центр предыдущего ромба, нужен чтобы дать необходимые точки,
// а те, которые посещали не посещать
CPoint CImage::getFinalPoint( const CPoint& center, const CPoint& currPoint, const TImage& otherImg )
{
	std::vector<CPoint> points = getPointsToCheck( center, currPoint );
	for( size_t i = 0; i < points.size(); ++i ) {
		double currError = calculateError( points[i], otherImg );
		if( currError < error ) {
			error = currError;
			minimumPoint = points[i];
		}
	}
	if( currentSP == SP_SDSP ) {
		// Нашли минимум в маленьком ромбе
		return minimumPoint;
	} else {
		if( currPoint == minimumPoint ) {
			currentSP = SP_SDSP;
			// Переходим к ромбу меньшего размера
		}
		return getFinalPoint( currPoint, minimumPoint, otherImg );
	}
}

double CImage::calculateError( const CPoint& coords, const TImage& otherImg ) const
{
	double sum = 0.0;
	for( int i = -radius; i < radius + 1; ++i ) {
		for( int j = -radius; j < radius + 1; ++j ) {
			int thisImgX = currentCenter.x + j;
			int thisImgY = currentCenter.y + i;
			int otherImgX = coords.x + j;
			int otherImgY = coords.y + i;
			sum += ( image[thisImgY][thisImgX] - otherImg[otherImgY][otherImgX] )
				* ( image[thisImgY][thisImgX] - otherImg[otherImgY][otherImgX] );
		}
	}

	return sum / ( radius * radius );
}

// Точки за рамкой (см конструктор) не берем
bool CImage::inTheBox( const CPoint& point ) const
{
	bool isInside = ( point.x < width - radius && point.x >= radius );
	isInside = isInside && ( point.y < height - radius && point.y >= radius );
	return isInside;
}

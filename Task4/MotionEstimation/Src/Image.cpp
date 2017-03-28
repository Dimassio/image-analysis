#include <Common.h>
#pragma hdrstop

#include <Image.h>
#include <Algorithm>
#include <cmath>

// Строим гистограмму по заданному набору значений и возвращаем
// максимальное значение
template<typename T>
T getHistogramMaximum( const std::vector<T>& values )
{
	std::vector<T> hist( values.begin(), values.end() );
	std::sort( hist.begin(), hist.end() );
	T maxValue = hist[0];
	int maxCounter = 0;
	int count = 1;
	T currValue = hist[0];
	for( size_t i = 1; i < hist.size(); ++i ) {
		if( abs( currValue - hist[i] ) < 0.000001 ) {
			count++;
		} else {
			if( count > maxCounter ) {
				maxCounter = count;
				maxValue = currValue;
			}
			currValue = hist[i];
			count = 1;
		}
	}
	return maxValue;
}

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
	int counter = 0;
	for( int i = radius; i < height - radius; i += radius * 2 + 1 ) {
		for( int j = radius; j < width - radius; j += radius * 2 + 1 ) {
			// todo: make good
			CBlock newBlock;
			newBlock.center = CPoint( j, i );
			blocks.push_back( newBlock );
			pointToBlockNumber.insert( std::make_pair( CPoint( j, i ), counter ) );
			counter++;
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
	for( size_t i = 0; i < blocks.size(); ++i ) {
		CPoint resultPoint = getBlockVector( blocks[i].center, other.image );
		// Меньшая ошибка - ошибка данного блока
		blocks[i].error = error;
		blocks[i].dest = resultPoint;
		CVector movement = resultPoint - blocks[i].center;
		// NOTE:  Т.к. ось У перевернута
		blocks[i].motionVector = CVector( movement.x, movement.y );
	}

	calculateVectorDev();
	calculateVariance();
	double maxBelief = calculateBeliefs();
	// 1 ая фильтрация. Выкидываем ненадежные блоки
	filterBlocks( maxBelief );
	int a, b;
	double s, phi;
	parametersEstimation( a, b, s, phi );
	return CVector( a, b );
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

// По описанию в TSearchPattern - возвращаем порядковый номер точки
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

// Получаем точки, которые нужно проверить.
// pointNumber - где был достигнут минимум
// центрируем ромб в эту точку и возвращаем точки,
// которые нужно проверить на следующем этапе
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

CPoint CImage::getBlockVector( const CPoint& block, const TImage& otherImg )
{
	CPoint startPoint = block;
	currentCenter = startPoint;
	currentSP = SP_LDSP;
	minimumPoint = block;
	error = INT_MAX;
	return getFinalPoint( block, block, otherImg );
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

// SAD
double CImage::calculateError( const CPoint& coords, const TImage& otherImg ) const
{
	double sum = 0.0;
	for( int i = -radius; i < radius + 1; ++i ) {
		for( int j = -radius; j < radius + 1; ++j ) {
			int thisImgX = currentCenter.x + j;
			int thisImgY = currentCenter.y + i;
			int otherImgX = coords.x + j;
			int otherImgY = coords.y + i;
			sum += abs( image[thisImgY][thisImgX] - otherImg[otherImgY][otherImgX] );
		}
	}

	return sum;
}

// Точки за рамкой (см конструктор) не берем
bool CImage::inTheBox( const CPoint& point ) const
{
	bool isInside = ( point.x < width - radius && point.x >= radius );
	isInside = isInside && ( point.y < height - radius && point.y >= radius );
	return isInside;
}

void CImage::calculateVectorDev()
{
	int kX = width / ( 2 * radius + 1 );
	int kY = height / ( 2 * radius + 1 );
	int lastPositionX = ( kX - 1 ) * ( 2 * radius + 1 ) + radius;
	int lastPositionY = ( kY - 1 ) * ( 2 * radius + 1 ) + radius;
	for( int i = radius; i < height - radius; i += radius * 2 + 1 ) {
		if( i == radius || i == lastPositionY ) {
			// Не понятно что делать с граничными блоками.
			// Потом отфильтруем их. См filterBlocks()
			continue;
		}
		for( int j = radius; j < width - radius; j += radius * 2 + 1 ) {
			if( j == radius || j == lastPositionX ) {
				// Не понятно что делать с граничными блоками.
				// Потом отфильтруем их. См filterBlocks()
				continue;
			}
			double sum = 0.0;
			int current = pointToBlockNumber[CPoint( j, i )];
			for( int deltaY = -1; deltaY <= 1; ++deltaY ) {
				if( deltaY == 0 ) {
					// Только с соседями
					continue;
				}
				for( int deltaX = -1; deltaX <= 1; ++deltaX ) {
					if( deltaX == 0 ) {
						// Только с соседями
						continue;
					}
					int neighbour = pointToBlockNumber[CPoint( j + deltaX * ( 2 * radius + 1 ), i + deltaY * ( 2 * radius + 1 ) )];
					sum += std::pow( blocks[current].motionVector.x - blocks[neighbour].motionVector.x, 2 )
						+ std::pow( blocks[current].motionVector.y - blocks[neighbour].motionVector.y, 2 );
				}
			}
			sum /= 4.0;
			blocks[current].dev = sum;
		}
	}
}

void CImage::calculateVariance()
{
	for( size_t iter = 0; iter < blocks.size(); ++iter ) {
		double disp = 0.0;
		int sum = 0;
		int sumSqr = 0;
		for( int deltaY = -radius; deltaY < radius + 1; ++deltaY ) {
			for( int deltaX = -radius; deltaX < radius + 1; ++deltaX ) {
				int thisImgX = blocks[iter].center.x + deltaX;
				int thisImgY = blocks[iter].center.y + deltaY;
				sum += image[thisImgY][thisImgX];
				sumSqr += image[thisImgY][thisImgX] * image[thisImgY][thisImgX];
			}
		}
		double mean = sum / ( ( 2 * radius + 1 ) * ( 2 * radius + 1 ) );
		disp = sumSqr / ( ( 2 * radius + 1 ) * ( 2 * radius + 1 ) ) - mean * mean;
		blocks[iter].disp = disp;
	}
}

double CImage::calculateBeliefs()
{
	double a = 0.25;
	double b = 32.0;
	double c = 1.0;
	double maxBelief = -INT_MAX;
	for( size_t i = 0; i < blocks.size(); ++i ) {
		if( abs( blocks[i].disp ) <= 0.000001 ) {
			blocks[i].belief = 0.0;
			continue;
		}
		double expr = a * blocks[i].error + b / blocks[i].disp + c * blocks[i].dev;
		blocks[i].belief = std::pow( expr, -1 );
		if( blocks[i].belief > maxBelief ) {
			maxBelief = blocks[i].belief;
		}
	}
	return maxBelief;
}

void CImage::filterBlocks( double maxBelief )
{
	double threshold = maxBelief;
	// Количество блоков не учитывая границы
	int numberOfBlocks = 0;
	int counter = 0;
	// При любой фильтрации хотим примерно половину полезных блоков оставлять
	while( counter <= numberOfBlocks / 2 && threshold >= 0 ) {
		counter = 0;
		numberOfBlocks = 0;
		threshold -= 0.01 * maxBelief;
		int kX = width / ( 2 * radius + 1 );
		int kY = height / ( 2 * radius + 1 );
		int lastPositionX = ( kX - 1 ) * ( 2 * radius + 1 ) + radius;
		int lastPositionY = ( kY - 1 ) * ( 2 * radius + 1 ) + radius;
		for( int i = radius; i < height - radius; i += radius * 2 + 1 ) {
			if( i == radius || i == lastPositionY ) {
				// Границы не берем
				continue;
			}
			for( int j = radius; j < width - radius; j += radius * 2 + 1 ) {
				if( j == radius || j == lastPositionX ) {
					// Границы не берем
					continue;
				}
				++numberOfBlocks;
				int current = pointToBlockNumber[CPoint( j, i )];
				if( blocks[current].belief >= threshold ) {
					counter++;
				}
			}
		}
	}

	refreshBlocks( threshold );
}

void CImage::refreshBlocks( double threshold )
{
	// Выкидываем границу,
	// и те блоки, belief которых меньше threshold
	std::vector<CBlock> newBlocks;
	int kX = width / ( 2 * radius + 1 );
	int kY = height / ( 2 * radius + 1 );
	int lastPositionX = ( kX - 1 ) * ( 2 * radius + 1 ) + radius;
	int lastPositionY = ( kY - 1 ) * ( 2 * radius + 1 ) + radius;
	for( int i = radius; i < height - radius; i += radius * 2 + 1 ) {
		if( i == radius || i == lastPositionY ) {
			continue;
		}
		for( int j = radius; j < width - radius; j += radius * 2 + 1 ) {
			if( j == radius || j == lastPositionX ) {
				continue;
			}
			int current = pointToBlockNumber[CPoint( j, i )];
			if( blocks[current].belief >= threshold ) {
				newBlocks.push_back( blocks[current] );
			}
		}
	}
	blocks = newBlocks;
}

void CImage::filterZ( double s )
{
	std::vector<CBlock> newBlocks;
	int currNumberOfBlocks = 0;
	double t = 0;
	// При любой фильтрации хотим примерно половину полезных блоков оставлять
	while( currNumberOfBlocks <= ( int ) blocks.size() / 2 ) {
		currNumberOfBlocks = 0;
		// По сотой от S изменяем
		t += 0.01 * s;
		for( size_t i = 0; i < blocks.size(); ++i ) {
			if( s - t <= blocks[i].z && blocks[i].z <= s + t ) {
				++currNumberOfBlocks;
			}
		}
	}
	for( size_t i = 0; i < blocks.size(); ++i ) {
		if( s - t <= blocks[i].z && blocks[i].z <= s + t ) {
			newBlocks.push_back( blocks[i] );
		}
	}

	blocks = newBlocks;
}

// Получаем параметры модели.
// Сначала получаем коэффициент масштаба S
// Затем можно найти поворот phi
// После чего, получим a & b. Т.к. у нас упрощенная модель,
// и не требуется находить ничего кроме сдвига, можем полагать phi = 0
void CImage::parametersEstimation( int& a, int& b, double& s, double& phi )
{
	int N = ( int ) blocks.size();
	// TODO: поиграй с ним
	int M = N / 2;
	// По этому набору будем смотрить гистограмму и находить S
	std::vector<double> z;
	for( int i = 0; i < N; ++i ) {
		// Для каждого вектора считаем Z_n
		std::vector<double> sValues;
		CBlock v1 = blocks[i];
		for( int j = 0; j < M; ++j ) {
			int vecNum = rand() % N;
			// Исключаем i вектор
			while( vecNum == i ) {
				vecNum = rand() % N;
			}
			CBlock v2 = blocks[vecNum];
			sValues.push_back( std::sqrt( ( double ) ( ( v2.dest.x - v1.dest.x ) * ( v2.dest.x - v1.dest.x ) + ( v2.dest.y - v1.dest.y ) * ( v2.dest.y - v1.dest.y ) )
				/ ( double ) ( ( v2.center.x - v1.center.x ) * ( v2.center.x - v1.center.x ) + ( v2.center.y - v1.center.y ) * ( v2.center.y - v1.center.y ) ) ) );
		}
		// Считаем максимум по одномерной гистограмме
		double zN = getHistogramMaximum( sValues );
		blocks[i].z = zN;
		z.push_back( zN );
	}
	// Зная все Z_n посчитаем параметр S
	s = getHistogramMaximum( z );
	// Фильтрация по z
	filterZ( s );
	// Считаем остальные параметры
	// Но сначала необходимые статистики
	int sumOfSqrX2 = 0;
	int sumOfSqrY2 = 0;
	int sumOfX2 = 0;
	int sumOfY2 = 0;
	int sumOfX1 = 0;
	int sumOfY1 = 0;
	int mixedProdSumXX12 = 0;
	int mixedProdSumXY12 = 0;
	int mixedProdSumYX12 = 0;
	int mixedProdSumYY12 = 0;
	for( size_t i = 0; i < blocks.size(); ++i ) {
		sumOfX1 += blocks[i].center.x;
		sumOfY1 += blocks[i].center.y;
		sumOfX2 += blocks[i].dest.x;
		sumOfY2 += blocks[i].dest.y;
		sumOfSqrX2 += blocks[i].dest.x * blocks[i].dest.x;
		sumOfSqrY2 += blocks[i].dest.y * blocks[i].dest.y;
		mixedProdSumXX12 += blocks[i].center.x * blocks[i].dest.x;
		mixedProdSumYY12 += blocks[i].center.y * blocks[i].dest.y;
		mixedProdSumXY12 += blocks[i].center.x * blocks[i].dest.y;
		mixedProdSumYX12 += blocks[i].center.y * blocks[i].dest.x;
	}
	// Количество блоков, после Z фильтрации
	int k = ( int ) blocks.size();
	// При желании, можно вычислить еще phi. 
	// Возможно ЛУЧШЕ сделать это, но что-то пошло не так.
	// double _x = k * sumOfSqrX2 + k * sumOfSqrY2 - sumOfX2 * sumOfX2 - sumOfY2 * sumOfY2;
	// double _y = sumOfX1 * sumOfX2 + sumOfX1 * sumOfY2 - sumOfY1 * sumOfX2 + sumOfY1 * sumOfY2
	// - k * mixedProdSumXX12 - k * mixedProdSumXY12 + k * mixedProdSumYX12 - k * mixedProdSumYY12;
	phi = 0.0;// std::acos( _x ) + std::acos( _s * _y );
	a = ( sumOfX2 - s * std::cos( phi ) * sumOfX1 + s * std::sin( phi ) * sumOfY1 ) / k;
	b = ( sumOfY2 - s * std::sin( phi ) * sumOfX1 - s * std::cos( phi ) * sumOfY1 ) / k;
}

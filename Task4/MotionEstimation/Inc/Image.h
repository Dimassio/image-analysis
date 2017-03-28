#pragma once

#include <vector>
#include <deque>
#include <map>

typedef std::vector< std::vector< BYTE > > TImage;

struct CPoint {
	int x;
	int y;

	CPoint( int _x = 0, int _y = 0 )
		: x( _x ), y( _y )
	{
	}

	CPoint& operator=( const CPoint& a )
	{
		if( &a == this ) {
			return *this;
		}
		x = a.x;
		y = a.y;
		return *this;
	}

	CPoint operator+( const CPoint& a ) const
	{
		return CPoint( a.x + x, a.y + y );
	}

	CPoint operator-( const CPoint& a ) const
	{
		return CPoint( x - a.x, y - a.y );
	}

	CPoint operator+=( const CPoint& a )
	{
		x += a.x;
		y += a.y;
		return *this;
	}

	bool operator==( const CPoint& a ) const
	{
		return ( x == a.x && y == a.y );
	}

	bool operator<( const CPoint& a ) const
	{
		if( x < a.x ) {
			return true;
		} else if( x == a.x ) {
			return y < a.y;
		} else {
			return false;
		}
	}
};

typedef CPoint CVector;

struct CBlock {
	CVector motionVector;
	CPoint center;
	CPoint dest;
	double error;
	double disp;
	double dev;
	double belief;
	// Используется при доп фильтрации + расчитывании зума
	double z;
};

class CImage {
public:
	explicit CImage( const BitmapData& bmpData );
	void GetData( BitmapData& data ) const;
	CVector EstimateMotionVectorFrom( const CImage& other );

private:
	enum TSearchPattern {
		// Виды поисковых шаблонов
		/*
		*1
		*8   *2
		*7   *0   *3
		*6   *4
		*5
		*/
		SP_LDSP,
		/*
		*1
		*4  *0  *2
		*3
		*/
		SP_SDSP
	};
	// Серое изображение
	TImage image;
	// Список блоков
	std::vector<CBlock> blocks;
	std::map<CPoint, int> pointToBlockNumber;
	CPoint currentCenter;
	TSearchPattern currentSP;
	double error = INT_MAX;
	double currBlockDisp;
	CPoint minimumPoint;
	int height;
	int width;
	// Окна 15 на 15
	const int radius = 7;
	// Список вершин, в которые возможно переходы из i центра
	std::vector< std::vector<int> > nodesToCheck;

	void initializeNodesToCheck();
	int getPointNumber( const CPoint& center, const CPoint& currPoint ) const;
	CPoint getNodePoint( const CPoint& center, int number ) const;
	std::vector<CPoint> getPointsToCheck( const CPoint& center, const CPoint& currPoint ) const;
	CPoint getBlockVector( const CPoint& block, const TImage& otherImg );
	CPoint getFinalPoint( const CPoint& center, const CPoint& currPoint, const TImage& otherImg );
	double calculateError( const CPoint& coords, const TImage& otherImg ) const;
	bool inTheBox( const CPoint& point ) const;
	void calculateVectorDev();
	void calculateVariance();
	double calculateBeliefs();
	void filterBlocks( double );
	void refreshBlocks( double threshold );
	void parametersEstimation( int& a, int& b, double& s, double& phi );
	void filterZ( double s );
};

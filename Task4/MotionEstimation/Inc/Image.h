#pragma once

#include <vector>
#include <map>

typedef std::vector< std::vector< BYTE > > TImage;

struct CPoint {
	int X;
	int Y;

	CPoint( int x = 0, int y = 0 ) :
		X( x ), Y( y )
	{
	}

	CPoint& operator=( const CPoint& a )
	{
		if( &a == this ) {
			return *this;
		}
		X = a.X;
		Y = a.Y;
		return *this;
	}

	CPoint operator+( const CPoint& a ) const
	{
		return CPoint( a.X + X, a.Y + Y );
	}

	CPoint operator-( const CPoint& a ) const
	{
		return CPoint( X - a.X, Y - a.Y );
	}

	CPoint operator+=( const CPoint& a )
	{
		X += a.X;
		Y += a.Y;
		return *this;
	}

	bool operator==( const CPoint& a ) const
	{
		return ( X == a.X && Y == a.Y );
	}

	bool operator<( const CPoint& a ) const
	{
		if( X < a.X ) {
			return true;
		} else if( X == a.X ) {
			return Y < a.Y;
		} else {
			return false;
		}
	}
};

typedef CPoint CVector;

struct CBlock {
	CVector MotionVector;
	CPoint Center;
	CPoint Dest;
	double Error;
	double Disp;
	double Dev;
	double Belief;
	// Используется при доп фильтрации + расчитывании зума
	double Z;
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
	const int radius = 9;
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

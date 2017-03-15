#pragma once

#include <vector>
#include <deque>

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
};

typedef CPoint CVector;

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
	std::vector<CPoint> blocks;
	CPoint currentCenter;
	TSearchPattern currentSP;
	double error = INT_MAX;
	CPoint minimumPoint;
	int height;
	int width;

	// Окна 15 на 15
	const int radius = 7;
	// Список вершин, в которые возможно переходы из i центра
	std::vector< std::vector<int> > nodesToCheck;

	void initializeNodesToCheck();
	// По описанию в TSearchPattern - порядковый номер точки
	int getPointNumber( const CPoint& center, const CPoint& currPoint ) const;
	CPoint getNodePoint( const CPoint& center, int number ) const;
	// Получаем точки, которые нужно проверить.
	// pointNumber - где был достигнут минимум
	// центрируем ромб в эту точку и возвращаем точки,
	// которые нужно проверить на следующем этапе
	std::vector<CPoint> getPointsToCheck( const CPoint& center, const CPoint& currPoint ) const;
	CVector getBlockVector( const CPoint& block, const TImage& otherImg );
	CPoint getFinalPoint( const CPoint& center, const CPoint& currPoint, const TImage& otherImg );
	double calculateError( const CPoint& coords, const TImage& otherImg ) const;
	bool inTheBox( const CPoint& point ) const;
};

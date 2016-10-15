#pragma once

#include <Common.h>
#include <windows.h>
#include <gdiplus.h>
#include <time.h>
#include <vector>

typedef std::vector< std::vector< COLORREF > > TImage;

using namespace Gdiplus;

// Класс обертка - представляет собой изображения в виде 2мерной матрицы, в ячейках которой
// лежат цвета в формате COLORREF. При создании объекта, матрица экстраполируется на 2 пикселя 
// с каждой стороны. По умолчанию эти пиксели просто дублируют соответсвтующие пиксели в строчках(столбцах)
class CBayerPattern {
public:
	explicit CBayerPattern( const BitmapData& bmpData, int defFrameValue = -1 );

	// Pixel Grouping алгоритм
	void Process();

	void GetData( BitmapData& bmpData ) const;
	std::vector<int> GetGist() const;

private:
	// Изображение размера (height + 4) X (width + 4)
	// так как не хочу думать что будет с крайними пикселями.
	// Со всех сторон на 2 строки пиксели просто дублируются.
	TImage image;
	size_t height;
	size_t width;

	void fillLeftRightEdges( int defFrameValue );
	void fillUpperAndLowerEdges( int defFrameValue );

	void restoreGreen();
	void restoreBlueRed();
	void computeRedBlueAtGreen();
	void computeBlueAtRed();
	void computeRedAtBlue();
	bool isBlueOnly( const size_t x, const size_t y ) const;
	bool isGreenOnly( const size_t x, const size_t y ) const;
	bool isRedOnly( const size_t x, const size_t y ) const;
	int hueTransit( int l1, int l2, int l3, int v1, int v3 ) const;

	void gammaCorrection( double gama );
};

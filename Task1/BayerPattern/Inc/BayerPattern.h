#pragma once

#include <Common.h>
#include <windows.h>
#include <gdiplus.h>
#include <time.h>
#include <vector>

typedef std::vector< std::vector< COLORREF > > TImage;

using namespace Gdiplus;

// ����� ������� - ������������ ����� ����������� � ���� 2������ �������, � ������� �������
// ����� ����� � ������� COLORREF. ��� �������� �������, ������� ���������������� �� 2 ������� 
// � ������ �������. �� ��������� ��� ������� ������ ��������� ��������������� ������� � ��������(��������)
class CBayerPattern {
public:
	explicit CBayerPattern( const BitmapData& bmpData, int defFrameValue = -1 );

	// Pixel Grouping ��������
	void Process();

	void GetData( BitmapData& bmpData ) const;
	std::vector<int> GetGist() const;

private:
	// ����������� ������� (height + 4) X (width + 4)
	// ��� ��� �� ���� ������ ��� ����� � �������� ���������.
	// �� ���� ������ �� 2 ������ ������� ������ �����������.
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

#pragma once

#include <vector>
#include <deque>

typedef std::vector< std::vector< BYTE > > TImage;

class CImage {
public:
	explicit CImage( const BitmapData& bmpData );
	// Возвращает уже БИНАРИЗОВАННОЕ изображение
	void GetData( BitmapData& data ) const;
	void NICKBinarization();

private:
	// Серое изображение
	TImage image;
	// Бинаризованного
	TImage binarizedImage;
	size_t height;
	size_t width;
	size_t zeroLevel;
	size_t filterSize;

	void fillUpperAndLowerEdges();
	void fillLeftRightEdges();
	void binarizePixel( size_t i, size_t j, BYTE threshold );
	void initializeTempBuffers( std::vector<int>& sumOfPixels, std::deque< std::vector<int> >& tempSum,
								  std::vector<int>& sumOfSqrPixels, std::deque< std::vector<int> >& tempSqrSum ) const;
};

#pragma once

#include <QtCore/QString>
#include <QtCore/QMap>

class HexFileReader
{
public:
	HexFileReader();
	bool readFile(const QString& filename);

	QList<uint32_t> segmentAddrs() const {
		return segments_.keys();
	}

	const QVector<uint8_t> data(uint32_t seg) const {
		return segments_[seg];
	}

private:
	uint8_t convertHexChar(const QChar &ch) {
		uint8_t ret = 0;

		if (ch.isDigit()) {
			ret = ch.toLatin1() - '0';
		}
		else if (ch >= 'a' && ch <= 'f') {
			ret = ch.toLatin1() + 10 - 'a';
		}
		else if (ch >= 'A' && ch <= 'F') {
			ret = ch.toLatin1() + 10 - 'A';
		}

		return ret;
	}

	bool processRecord(uint8_t len, uint16_t addr, uint8_t type, const QVector<uint8_t>& data);
	uint8_t computeSum(uint8_t len, uint16_t addr, uint8_t type, const QVector<uint8_t>& data);
	uint32_t extractHexChars(const QString& line, int pos, int length);
	bool processLine(const QString& line);

	uint32_t swapBytes(uint32_t v) {
		return ((v >> 8) & 0xff) | ((v << 8) & 0xFF00);
	}

	void insertAt(uint32_t addr, const QVector<uint8_t>& data);

private:
	uint32_t baseaddr_;
	uint32_t lastaddr_;
	QVector<uint8_t> data_;

	QMap<uint32_t, QVector<uint8_t>> segments_;
};

#include "HexFileReader.h"
#include <QtCore/QFile>
#include <QtCore/QTextStream>

HexFileReader::HexFileReader()
{
	baseaddr_ = 0;
	lastaddr_ = 0;
}

uint32_t HexFileReader::extractHexChars(const QString& line, int pos, int len)
{
	uint32_t ret = 0;

	while (len-- > 0) {
		ret = ret << 4;
		ret |= convertHexChar(line[pos++]);
	}

	return ret;
}

uint8_t HexFileReader::computeSum(uint8_t len, uint16_t addr, uint8_t type, const QByteArray& data)
{
	uint8_t v = 0;

	v += len;
	v += (addr & 0xFF);
	v += ((addr >> 8) & 0xFF);
	v += type;

	for (int i = 0; i < data.length(); i++) {
		v += data.at(i);
	}

	return (~v) + 1;
}

bool HexFileReader::processRecord(uint8_t len, uint16_t addr, uint8_t type, const QByteArray& data)
{
	bool ret = true;

	if (type == 0) {
		// Data Record
		uint32_t hereaddr = baseaddr_ + addr;
		insertAt(hereaddr, data);
	}
	else if (type == 4) {
		// Extended Linear Address Record
		if (data_.length() > 0) {
			segments_.insert(baseaddr_, data_);
		}

		baseaddr_ = (data[0] << 24) | (data[1] << 16);
		lastaddr_ = baseaddr_;
		data_.clear();
	}
	else if (type == 1) {
		// Ignore this one
	}
	else {
		ret = false;
	}

	return ret;
}

void HexFileReader::insertAt(uint32_t addr, const QByteArray& data)
{
	bool found = false;

	for (uint32_t base : segments_.keys()) {
		uint32_t endseg = base + segments_[base].count();
		if (addr >= base && addr <= endseg) {
			if (addr == endseg) {
				//
				// Just append
				//
				segments_[base].append(data);
				return;
			}
			else {
				assert(false);
			}
		}
	}

	if (!found) {
		segments_.insert(addr, data);
	}
}


bool HexFileReader::processLine(const QString& line)
{
	uint32_t index = 1;
	QByteArray data;

	if (line[0] != ':')
		return true;

	uint32_t ll = extractHexChars(line, index, 2);
	index += 2;

	uint32_t aaaa = extractHexChars(line, index, 4);
	index += 4;

	uint32_t tt = extractHexChars(line, index, 2);
	index += 2;

	for (int i = 0; i < ll; i++) {
		uint32_t d = extractHexChars(line, index, 2);
		index += 2;
		data.push_back(static_cast<uint8_t>(d));
	}

	uint8_t fsum = extractHexChars(line, index, 2);
	uint8_t csum = computeSum(ll, aaaa, tt, data);

	if (fsum != csum)
		return false;

	return processRecord(ll, aaaa, tt, data);
}

bool HexFileReader::readFile(const QString& filename)
{
	QFile file(filename);
	if (!file.open(QIODevice::ReadOnly))
		return false;

	QTextStream strm(&file);
	while (!strm.atEnd())
	{
		QString line = strm.readLine();
		processLine(line);
	}

	file.close();

	return true;
}
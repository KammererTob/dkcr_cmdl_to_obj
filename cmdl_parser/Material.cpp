#include "Material.h"


Material::Material(std::string materialName)
{
	this->materialName = materialName;
	this->materialDefinition << "newmtl " << materialName << std::endl;
}


Material::Material(uint32_t vertexAttributeFlags, uint64_t textureId)
{
	this->textureId = textureId;
	this->vertexAttributeFlags = vertexAttributeFlags;
}

Material::~Material()
{
}


void Material::convertTXTRtoDDS(const std::string &textureDir /*= ""*/)
{
	std::stringstream ss;
	ss << std::hex << this->textureId << std::dec;
	std::string fileId = ss.str();
	while (fileId.length() < 16) {
		ss.str("");
		ss << "0" << fileId;
		fileId = ss.str();
	}
	std::ifstream txtrFile(textureDir + fileId + ".TXTR", std::ifstream::binary);

	if (!txtrFile.fail() && txtrFile.is_open()) {
		uint32_t txFormat, numMipMaps;
		uint16_t width, height;

		// Texture Format
		txtrFile.read(reinterpret_cast<char *>(&txFormat), sizeof(txFormat));
		txFormat = _byteswap_ulong(txFormat);
		if (txFormat != 0xA) {
			std::cout << "Unsupported Texture Format: 0x" << std::hex << txFormat << std::dec << std::endl;
			return;
		}

		// Width and Height
		txtrFile.read(reinterpret_cast<char *>(&width), sizeof(width));
		width = _byteswap_ushort(width);
		txtrFile.read(reinterpret_cast<char *>(&height), sizeof(height));
		height = _byteswap_ushort(height);

		// Number of MipMaps
		txtrFile.read(reinterpret_cast<char *>(&numMipMaps), sizeof(numMipMaps));
		numMipMaps = _byteswap_ulong(numMipMaps);

		// Write the File
		std::ofstream ddsFile(textureDir + "dds/" + fileId + ".dds", std::ofstream::binary);

		// First the header [https://msdn.microsoft.com/en-us/library/windows/desktop/bb943982(v=vs.85).aspx]. 
		// This is fugly.. maybe revise this later...
		uint32_t number = 0x20534444;
		ddsFile.write(reinterpret_cast<const char *>(&number), sizeof(uint32_t));
		number = 0x7C;
		ddsFile.write(reinterpret_cast<const char *>(&number), sizeof(uint32_t));
		number = 0x021007;
		ddsFile.write(reinterpret_cast<const char *>(&number), sizeof(uint32_t));
		number = height;
		ddsFile.write(reinterpret_cast<const char *>(&number), sizeof(uint32_t));
		number = width;
		ddsFile.write(reinterpret_cast<const char *>(&number), sizeof(uint32_t));
		number = (height * width / 2);
		ddsFile.write(reinterpret_cast<const char *>(&number), sizeof(uint32_t));
		number = 0;
		ddsFile.write(reinterpret_cast<const char *>(&number), sizeof(uint32_t));
		number = numMipMaps;
		ddsFile.write(reinterpret_cast<const char *>(&number), sizeof(uint32_t));
		for (int i = 0; i < 11; i++) {
			number = 0;
			ddsFile.write(reinterpret_cast<const char *>(&number), sizeof(uint32_t));
		}
		number = 0x20;
		ddsFile.write(reinterpret_cast<const char *>(&number), sizeof(uint32_t));
		number = 0x4;
		ddsFile.write(reinterpret_cast<const char *>(&number), sizeof(uint32_t));
		number = 0x31545844;
		ddsFile.write(reinterpret_cast<const char *>(&number), sizeof(uint32_t));
		for (int i = 0; i < 5; i++) {
			number = 0;
			ddsFile.write(reinterpret_cast<const char *>(&number), sizeof(uint32_t));
		}
		number = 0x401000;
		ddsFile.write(reinterpret_cast<const char *>(&number), sizeof(uint32_t));
		for (int i = 0; i < 4; i++) {
			number = 0;
			ddsFile.write(reinterpret_cast<const char *>(&number), sizeof(uint32_t));
		}

		// Make sure we are at the end of the header..
		txtrFile.seekg(0xC, txtrFile.beg);
		int buffersize = (width * height) / 2;
		int mipMapSize = 1;
		for (unsigned int i = 0; i < numMipMaps; i++) {
			std::vector<uint8_t> TXTRByteArray;
			for (int byteCount = 0; byteCount < (buffersize / std::pow(mipMapSize, 2)); byteCount++) {
				uint8_t readByte;
				txtrFile.read(reinterpret_cast<char *>(&readByte), sizeof(readByte));
				TXTRByteArray.push_back(readByte);
			}
			int mipWidth = width / mipMapSize;
			int mipHeight = height / mipMapSize;
			this->uncompressBytesAndWrite(TXTRByteArray, mipWidth, mipHeight, ddsFile);
			mipMapSize *= 2;
		}

		ddsFile.close();

	}
	else {
		char errBuff[256];
		strerror_s(errBuff, 100, errno);
		std::cout << "Opening TXTR File failed. Error: " << errBuff << std::endl;
		return;
	}

	std::cout << "Texture Conversion successful!" << std::endl;
	txtrFile.close();
}

void Material::setTextureId(uint64_t textureId)
{
	this->textureId = textureId;
}

uint64_t Material::getTextureId() const
{
	return this->textureId;
}

void Material::setVertexAttributeFlags(uint32_t flags)
{
	this->vertexAttributeFlags = flags;
}

uint32_t Material::getVertexAttributeFlags() const
{
	return this->vertexAttributeFlags;
}

std::string Material::getMaterialName() const
{
	return this->materialName;
}

std::string Material::getMaterialDefinition() const
{
	return this->materialDefinition.str();
}

// Thanks to Thakis and Parax
void Material::uncompressBytesAndWrite(std::vector<uint8_t> &byteArray, int width, int height, std::ofstream &outFile)
{
	std::vector<uint8_t> outByteArray(byteArray.size());

	int s = 0;
	int y = 0;
	int height_offset = height / 4;
	int width_offset = width / 4;

	while (y < height_offset) {
		int x = 0;

		while (x < width_offset) {
			int dy = 0;

			while (dy < 2) {
				int dx = 0;
				
				while (dx < 2) {
					for (int byteCount = 0; byteCount < 8; byteCount++) {
						int arrayIndex = 8 * (((y + dy) * width_offset) + x + dx) + byteCount;
						outByteArray[arrayIndex] = byteArray[s + byteCount];
					}
					s += 8;
					dx += 1;
				}
				dy += 1;
			}
			x += 2;
		}
		y += 2;
	}

	for (unsigned int byteCount = 0; byteCount < (outByteArray.size() / 8); byteCount++) {

		int chunknum = 8 * byteCount; // index at 0

		uint8_t b1 = outByteArray[chunknum];
		uint8_t b2 = outByteArray[chunknum + 1];
		uint8_t b3 = outByteArray[chunknum + 2];
		uint8_t b4 = outByteArray[chunknum + 3];
		uint8_t b5 = outByteArray[chunknum + 4];
		uint8_t b6 = outByteArray[chunknum + 5];
		uint8_t b7 = outByteArray[chunknum + 6];
		uint8_t b8 = outByteArray[chunknum + 7];

		uint8_t number;
		number = b2;
		outFile.write(reinterpret_cast<const char *>(&number), sizeof(uint8_t));
		number = b1;
		outFile.write(reinterpret_cast<const char *>(&number), sizeof(uint8_t));
		number = b4;
		outFile.write(reinterpret_cast<const char *>(&number), sizeof(uint8_t));
		number = b3;
		outFile.write(reinterpret_cast<const char *>(&number), sizeof(uint8_t));
		//number = this->reverseBits(b5);
		number = this->swapBits(b5);
		outFile.write(reinterpret_cast<const char *>(&number), sizeof(uint8_t));
		//number = this->reverseBits(b6);
		number = this->swapBits(b6);
		outFile.write(reinterpret_cast<const char *>(&number), sizeof(uint8_t));
		//number = this->reverseBits(b7);
		number = this->swapBits(b7);
		outFile.write(reinterpret_cast<const char *>(&number), sizeof(uint8_t));
		//number = this->reverseBits(b8);
		number = this->swapBits(b8);
		outFile.write(reinterpret_cast<const char *>(&number), sizeof(uint8_t));
	}
}

unsigned char Material::reverseBits(unsigned char b)
{
	b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
	b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
	b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
	return b;
}

unsigned char Material::swapBits(unsigned char b)
{
	unsigned char b1 = b & 0x3;
	unsigned char b2 = b & 0xC;
	unsigned char b3 = b & 0x30;
	unsigned char b4 = b & 0xC0;
	b = (b1 << 6) + (b2 << 2) + (b3 >> 2) + (b4 >> 6);
	return b;
}

std::string Pass::parseSection(Material &material, char* buffer, int size)
{
	uint64_t textureFileId;
	memcpy(&textureFileId, buffer + 8, 8);
	textureFileId = _byteswap_uint64(textureFileId);
	std::cout << "Texture File ID: " << std::hex << textureFileId << std::dec << std::endl;

	material.setTextureId(textureFileId);
	// Convert the Texture to DDS
	material.convertTXTRtoDDS("Textures/"); // With trailing slash /

	std::stringstream passSection;
	passSection << "Kd 1.000 1.000 1.000" << std::endl;
	passSection << "map_Kd " << std::hex << textureFileId << ".dds" << std::endl;

	return passSection.str();
}

std::string SectionType::parseSection(Material &material, char* buffer, int size)
{
	std::cout << "Default parse. SHOULD NEVER HAPPEN!" << std::endl;
	exit(-1);
}

std::string Clr::parseSection(Material &material, char* buffer, int size)
{
	std::cout << "CLR not implemented" << std::endl;
	return "";
}

std::string Int::parseSection(Material &material, char* buffer, int size)
{
	std::cout << "INT not implemented" << std::endl;
	return "";
}

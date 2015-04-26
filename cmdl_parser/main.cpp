#include <iostream>
#include <fstream>
#include <vector>
#include <stdint.h>
#include <iomanip>
#include <sstream>

struct float3 {
	float x;
	float y;
	float z;
};

struct float2 {
	float u;
	float v;
};

struct IndexTriplet {
	unsigned short pos;
	unsigned short norm;
	unsigned short tex;
};

struct Vertex {
	float3 position;
	float3 normal;
	float2 uv;
};

struct CMDL_HEADER
{
	// 0x00
	uint32_t flags;
	float3 boundingBox[2];
	uint32_t sectionCount; // includes header count
	uint32_t headerCount; // number of header sections
	uint32_t materialSetCount;
	std::vector<uint32_t> sectionSizes;
} fileHeader;


inline void toDWORD(uint32_t& d)
{
	uint8_t w1 = d & 0xff;
	uint8_t w2 = (d >> 8) & 0xff;
	uint8_t w3 = (d >> 16) & 0xff;
	uint8_t w4 = d >> 24;
	d = (w1 << 24) | (w2 << 16) | (w3 << 8) | w4;
}

inline void toWORD(uint16_t& d)
{
	uint8_t w1 = d & 0xff;
	uint8_t w2 = (d >> 8) & 0xff;
	d = (w1 << 8) | w2;
}

float FloatSwap(float f)
{
	union
	{
		float f;
		unsigned char b[4];
	} dat1, dat2;

	dat1.f = f;
	dat2.b[0] = dat1.b[3];
	dat2.b[1] = dat1.b[2];
	dat2.b[2] = dat1.b[1];
	dat2.b[3] = dat1.b[0];
	return dat2.f;
}


void main(int argc, char* argv[])
{
	char errBuff[256];
	char* fileName;
	if (argc < 2) {
		std::cout << "No Input file. Using Testfile" << std::endl;
		fileName = "testing.CMDL";
	}
	else {
		fileName = argv[1];
	}

	std::ifstream inputFile(fileName, std::ifstream::in | std::ifstream::binary);

	if (inputFile.fail()) {
		strerror_s(errBuff, 100, errno);
		std::cout << "Failed to open input file (ErrorCode: " << errBuff  << ")" << std::endl;
		exit(0);
	}
	uint32_t toWaste;
	inputFile.read(reinterpret_cast<char *>(&toWaste), sizeof(toWaste));

	inputFile.read(reinterpret_cast<char *>(&fileHeader.flags), sizeof(toWaste));
	toDWORD(fileHeader.flags);
	std::cout << "Header Flags: " << std::hex << fileHeader.flags << std::dec << std::endl;

	// Min Bounding Box
	inputFile.read(reinterpret_cast<char *>(&fileHeader.boundingBox[0].x), sizeof(float));
	inputFile.read(reinterpret_cast<char *>(&fileHeader.boundingBox[0].y), sizeof(float));
	inputFile.read(reinterpret_cast<char *>(&fileHeader.boundingBox[0].z), sizeof(float));

	// Max Bounding Box
	inputFile.read(reinterpret_cast<char *>(&fileHeader.boundingBox[1].x), sizeof(float));
	inputFile.read(reinterpret_cast<char *>(&fileHeader.boundingBox[1].y), sizeof(float));
	inputFile.read(reinterpret_cast<char *>(&fileHeader.boundingBox[1].z), sizeof(float));

	// Section Count
	inputFile.read(reinterpret_cast<char *>(&fileHeader.sectionCount), sizeof(fileHeader.sectionCount));
	toDWORD(fileHeader.sectionCount);
	std::cout << "Section Count: " << fileHeader.sectionCount << std::endl;

	// Material Set Count
	inputFile.read(reinterpret_cast<char *>(&fileHeader.materialSetCount), sizeof(fileHeader.materialSetCount));
	toDWORD(fileHeader.materialSetCount);
	std::cout << "Material-Set Count: " << fileHeader.materialSetCount << std::endl;

	int visibilityGroupSize = 0;
	if ((fileHeader.flags & 0x10) == 0x10) { // We need to parse/skip visibility groups...
		inputFile.ignore(4); // Ignore Unknown bytes.
		uint32_t visGroupCount;
		inputFile.read(reinterpret_cast<char *>(&visGroupCount), sizeof(visGroupCount)); // We need this to skip the right amount of bytes...
		toDWORD(visGroupCount);
		std::cout << "Visibility Group Count: " << visGroupCount << std::endl;
		visibilityGroupSize = 8;
		for (unsigned int i = 0; i < visGroupCount; i++) {
			std::cout << '\t';
			uint32_t visGroupNameLength;
			inputFile.read(reinterpret_cast<char *>(&visGroupNameLength), sizeof(visGroupNameLength));
			toDWORD(visGroupNameLength);

			char* visGroupName = new char[visGroupNameLength];
			inputFile.read(visGroupName, visGroupNameLength);
			std::cout << visGroupName << std::endl;

			delete visGroupName;
			visibilityGroupSize += (4 + visGroupNameLength);
		}
		inputFile.ignore(20); // Ignore the last 20 bytes (unknown)
		visibilityGroupSize += 20;
	}
	// Section Sizes
	for (unsigned int i = 0; i < fileHeader.sectionCount; i++) {
		uint32_t sectionSize;
		inputFile.read(reinterpret_cast<char *>(&sectionSize), sizeof(sectionSize));
		toDWORD(sectionSize);
		fileHeader.sectionSizes.push_back(sectionSize);
	}

	std::cout << "Section Sizes: " << std::endl;
	for (unsigned int i = 0; i < fileHeader.sectionCount; i++) {
		std::cout << "\tSection" << i << ": " << fileHeader.sectionSizes[i] << std::endl;
	}

	int headerSize = 40 + visibilityGroupSize + fileHeader.sectionCount * 4;
	inputFile.ignore(32 - (headerSize % 32)); // Pad this shit...

	// For Debug Purposes
	std::streamoff curFp = inputFile.tellg();
	for (unsigned int i = 0; i < fileHeader.sectionCount; i++) {
		std::stringstream fileName;
		fileName << "Section" << i << ".sec";
		std::ofstream debugFiles(fileName.str());

		char* fileBuffer = new char[fileHeader.sectionSizes[i]];
		inputFile.read(fileBuffer, fileHeader.sectionSizes[i]);
		debugFiles.write(fileBuffer, fileHeader.sectionSizes[i]);

		debugFiles.close();
		delete fileBuffer;
	}
	inputFile.seekg(curFp, inputFile.beg);

	// Read Materials for now..
	uint32_t sizeRemainder = fileHeader.sectionSizes[0] - 4;

	uint32_t materialCount;
	inputFile.read(reinterpret_cast<char *>(&materialCount), sizeof(materialCount));
	toDWORD(materialCount);

	std::vector<uint32_t> vertexAttributesFlags;
	for (unsigned int i = 0; i < materialCount; i++) {
		uint32_t mFlags, mSize;
		inputFile.read(reinterpret_cast<char *>(&mSize), sizeof(mSize));
		toDWORD(mSize);
		sizeRemainder -= (mSize + 4);
		inputFile.ignore(12);
		inputFile.read(reinterpret_cast<char *>(&mFlags), sizeof(mFlags));
		toDWORD(mFlags);
		std::cout << "Flag" << i << ": " << std::hex << mFlags << std::dec << std::endl;
		vertexAttributesFlags.push_back(mFlags);
		inputFile.ignore(mSize - 16);	 // -20 because -4 + -12 (we ignored) + -4 for the flags
	}
	inputFile.ignore(sizeRemainder);


	std::ofstream outFile("check.obj");
	outFile << "#" << std::endl << "#" << std::endl;
	// Get the Vertex coordinates.
	std::vector<float3> positions;
	uint32_t numVerts = fileHeader.sectionSizes[1] / 12; // 12 (3 float a 4 bytes) per Vertex
	if ((fileHeader.flags & 0x20) == 0x20) {
		numVerts = fileHeader.sectionSizes[1] / 6; // 6 (3 shorts a 2 bytes) per Vertex
	}
	for (unsigned int i = 0; i < numVerts; i++) {
		float3 vertexPos;
		if ((fileHeader.flags & 0x20) == 0x20) {
			uint16_t x, y, z;
			inputFile.read(reinterpret_cast<char *>(&x), sizeof(x));
			inputFile.read(reinterpret_cast<char *>(&y), sizeof(y));
			inputFile.read(reinterpret_cast<char *>(&z), sizeof(z));
			toWORD(x);
			toWORD(y);
			toWORD(z);
			vertexPos.x = (float)x / 0x2000;
			vertexPos.y = (float)y / 0x2000;
			vertexPos.z = (float)z / 0x2000;
		}
		else {
			inputFile.read(reinterpret_cast<char *>(&vertexPos.x), sizeof(vertexPos.x));
			inputFile.read(reinterpret_cast<char *>(&vertexPos.y), sizeof(vertexPos.y));
			inputFile.read(reinterpret_cast<char *>(&vertexPos.z), sizeof(vertexPos.z));
			vertexPos.x = FloatSwap(vertexPos.x);
			vertexPos.y = FloatSwap(vertexPos.y);
			vertexPos.z = FloatSwap(vertexPos.z);
		}
		outFile << "v " << vertexPos.x << " " << vertexPos.y << " " << vertexPos.z << std::endl;
	}
	if (numVerts > 0) {
		inputFile.ignore(fileHeader.sectionSizes[1] % numVerts);
	}
	else {
		inputFile.ignore(fileHeader.sectionSizes[1]);
	}

	// Get Vertex Normals
	std::vector<float3> normals;
	uint32_t numNormals = fileHeader.sectionSizes[2] / 6; // 6 (3 short a 2 bytes) per Vertex
	for (unsigned int i = 0; i < numNormals; i++) {
		float3 normal;
		uint16_t x, y, z;
		inputFile.read(reinterpret_cast<char *>(&x), sizeof(x));
		inputFile.read(reinterpret_cast<char *>(&y), sizeof(y));
		inputFile.read(reinterpret_cast<char *>(&z), sizeof(z));
		toWORD(x);
		toWORD(y);
		toWORD(z);
		normal.x = (float)x / 0x8000;
		normal.y = (float)y / 0x8000;
		normal.z = (float)z / 0x8000;
		outFile << "vn " << normal.x << " " << normal.y << " " << normal.z << std::endl;
	}
	if (numNormals > 0) {
		inputFile.ignore(fileHeader.sectionSizes[2] % numNormals);
	}
	else {
		inputFile.ignore(fileHeader.sectionSizes[2]);
	}

	inputFile.ignore(fileHeader.sectionSizes[3]);
	inputFile.ignore(fileHeader.sectionSizes[4]);

	// Get Vertex UVs
	std::vector<float2> uvs;
	uint32_t numUvs = fileHeader.sectionSizes[5] / 4; // 4 (2 short a 2 bytes) per Vertex
	for (unsigned int i = 0; i < numUvs; i++) {
		uint16_t u, v;
		inputFile.read(reinterpret_cast<char *>(&u), sizeof(u));
		inputFile.read(reinterpret_cast<char *>(&v), sizeof(v));
		float2 uv;
		toWORD(u);
		toWORD(v);
		uv.u = (float)u / 0x2000;
		uv.v = (float)v / 0x2000;
		outFile << "vt " << uv.u << " " << uv.v <<  std::endl;
	}
	if (numUvs > 0) {
		inputFile.ignore(fileHeader.sectionSizes[5] % numUvs);
	}
	else {
		inputFile.ignore(fileHeader.sectionSizes[5]);
	}


	inputFile.ignore(fileHeader.sectionSizes[6]);

	// Get all Submeshes..
	for (unsigned int i = 7; i < fileHeader.sectionSizes.size(); i++) {
		bool hasNrm = false;
		int bytesRead = 0;
		int bytesToSkip = 0;
		int numColors, numUVs = 0;
		inputFile.ignore(0x1A);
		bytesRead += 0x1A;
		uint16_t matID;
		inputFile.read(reinterpret_cast<char *>(&matID), sizeof(matID));
		bytesRead += 2;
		toWORD(matID);

		if ((vertexAttributesFlags[matID] & 0x3) != 0x3) {
			std::cout << "FATAAAAAAL" << std::endl;
			exit(-1);
		}
		inputFile.ignore(2);
		uint16_t unknownFlag;
		inputFile.read(reinterpret_cast<char *>(&unknownFlag), sizeof(unknownFlag));
		toWORD(unknownFlag);
		bytesRead += 4;


		if ((vertexAttributesFlags[matID] & 0xFF000000) == 0x1000000) bytesToSkip = 1;	// Don't know if this is completely true yet, but this happens from time to time...
		if ((vertexAttributesFlags[matID] & 0xFF000000) == 0x3000000) bytesToSkip = 2; 
		if ((vertexAttributesFlags[matID] & 0xC) == 0xC) hasNrm = true;

		switch (vertexAttributesFlags[matID] & 0xF0) {
		case 0x0:
			numColors = 0;
			break;
		case 0x30:
			numColors = 1;
			break;
		case 0xC0:
			numColors = 1;
			break;
		case 0xF0:
			numColors = 2;
			break;
		}

		switch (vertexAttributesFlags[matID] & 0x3FFF00) {
		case 0x0: 
			numUVs = 0;
			break;
		case 0x300:
			numUVs = 1;
			break;
		case 0xF00: 
			numUVs = 2;
			break;
		case 0x3F00: 
			numUVs = 3;
			break;
		case 0xFF00: 
			numUVs = 4;
			break;
		case 0x3FF00: 
			numUVs = 5;
			break;
		case 0xFFF00: 
			numUVs = 6;
			break;
		case 0x3FFF00: 
			numUVs = 7;
			break;
		default: 
			numUVs = 2;
			break;
		}

		uint8_t primitveFlag = 1;

		while (primitveFlag != 0) {
			inputFile.read(reinterpret_cast<char *>(&primitveFlag), sizeof(primitveFlag));
			bytesRead++;
			if (primitveFlag == 0) { // This could already be the header of the next section...
				inputFile.seekg(-1, inputFile.cur); // Set the file pointer 1 byte back to we don't get a corrupted header if this was the next section
				bytesRead--;
				break;
			}

			uint16_t primitiveObjectCount;
			inputFile.read(reinterpret_cast<char *>(&primitiveObjectCount), sizeof(primitiveObjectCount));
			bytesRead += 2;
			toWORD(primitiveObjectCount);

			std::vector<uint16_t> curVertices;
			std::vector<uint16_t> curNormals;
			std::vector<uint16_t> curUvs;

			switch (primitveFlag & 0xF8) {
			case 0x90: // Triangles
				for (unsigned int x = 0; x < (primitiveObjectCount / 3); x++) {
					outFile << "f ";
					for (unsigned y = 0; y < 3; y++) {

						inputFile.ignore(bytesToSkip);
						bytesRead += bytesToSkip;

						uint16_t indexPosition, indexNormal, indexUV;
						inputFile.read(reinterpret_cast<char *>(&indexPosition), sizeof(indexPosition));
						bytesRead += 2;
						toWORD(indexPosition);

						if (hasNrm) {
							inputFile.read(reinterpret_cast<char *>(&indexNormal), sizeof(indexNormal));
							bytesRead += 2;
							toWORD(indexNormal);
						}

						inputFile.ignore(2 * numColors); // Ignore the Color Values (most of the time there aren't present anyways..
						bytesRead += 2 * numColors;

						if (numUVs >= 1) {
							inputFile.read(reinterpret_cast<char *>(&indexUV), sizeof(indexUV));
							bytesRead += 2;
							toWORD(indexUV);
						}

						outFile << indexPosition+1 << "/" << (numUVs >= 1 ? std::to_string(indexUV+1) : "") << "/" << (hasNrm ? std::to_string(indexNormal+1) : "") << " ";
					}
					outFile << std::endl;
				}
				break;
			case 0x98: // Triangle Strip
				for (unsigned x = 0; x < primitiveObjectCount; x++) {
					inputFile.ignore(bytesToSkip);
					bytesRead += bytesToSkip;

					uint16_t indexPosition, indexNormal, indexUV;
					inputFile.read(reinterpret_cast<char *>(&indexPosition), sizeof(indexPosition));
					bytesRead += 2;
					toWORD(indexPosition);

					if (hasNrm) {
						inputFile.read(reinterpret_cast<char *>(&indexNormal), sizeof(indexNormal));
						bytesRead += 2;
						toWORD(indexNormal);
					}

					inputFile.ignore(2 * numColors); // Ignore the Color Values (most of the time there aren't present anyways..
					bytesRead += 2 * numColors;

					if (numUVs >= 1) {
						inputFile.read(reinterpret_cast<char *>(&indexUV), sizeof(indexUV));
						bytesRead += 2;
						toWORD(indexUV);
					}

					curVertices.push_back(indexPosition);
					curNormals.push_back(indexNormal);
					curUvs.push_back(indexUV);
				}

				for (unsigned x = 2; x < curVertices.size(); x++) {
					// We do we do this?
					outFile << "f ";
					outFile << curVertices[x - 2] + 1 << "/" << (numUVs >= 1 ? std::to_string(curUvs[x - 2] + 1) : "") << "/" << (hasNrm ? std::to_string(curNormals[x - 2] + 1) : "") << " ";
					outFile << curVertices[x - 1] + 1 << "/" << (numUVs >= 1 ? std::to_string(curUvs[x - 1] + 1) : "") << "/" << (hasNrm ? std::to_string(curNormals[x - 1] + 1) : "") << " ";
					outFile << curVertices[x] + 1 << "/" << (numUVs >= 1 ? std::to_string(curUvs[x] + 1) : "") << "/" << (hasNrm ? std::to_string(curNormals[x] + 1) : "") << " ";
					outFile << std::endl;
				}
				break;
			case 0xA0: // Triangle Fan
				// First we try it fucking primitive :P
				inputFile.ignore(bytesToSkip);
				bytesRead += bytesToSkip;

				uint16_t centerindexPosition, centerindexNormal, centerindexUV;
				inputFile.read(reinterpret_cast<char *>(&centerindexPosition), sizeof(centerindexPosition));
				bytesRead += 2;
				toWORD(centerindexPosition);

				if (hasNrm) {
					inputFile.read(reinterpret_cast<char *>(&centerindexNormal), sizeof(centerindexNormal));
					bytesRead += 2;
					toWORD(centerindexNormal);
				}

				inputFile.ignore(2 * numColors); // Ignore the Color Values (most of the time there aren't present anyways..
				bytesRead += 2 * numColors;

				if (numUVs >= 1) {
					inputFile.read(reinterpret_cast<char *>(&centerindexUV), sizeof(centerindexUV));
					bytesRead += 2;
					toWORD(centerindexUV);
				}

				for (unsigned int u = 1; u < primitiveObjectCount; u++) {

					inputFile.ignore(bytesToSkip);
					bytesRead += bytesToSkip;

					uint16_t indexPosition, indexNormal, indexUV;
					inputFile.read(reinterpret_cast<char *>(&indexPosition), sizeof(indexPosition));
					bytesRead += 2;
					toWORD(indexPosition);

					if (hasNrm) {
						inputFile.read(reinterpret_cast<char *>(&indexNormal), sizeof(indexNormal));
						bytesRead += 2;
						toWORD(indexNormal);
					}

					inputFile.ignore(2 * numColors); // Ignore the Color Values (most of the time there aren't present anyways..
					bytesRead += 2 * numColors;

					if (numUVs >= 1) {
						inputFile.read(reinterpret_cast<char *>(&indexUV), sizeof(indexUV));
						bytesRead += 2;
						toWORD(indexUV);
					}

					curVertices.push_back(indexPosition);
					curNormals.push_back(indexNormal);
					curUvs.push_back(indexUV);
				}

				for (unsigned int l = 1; l < curVertices.size(); l++) {
					outFile << "f ";
					outFile << centerindexPosition + 1 << "/" << (numUVs >= 1 ? std::to_string(centerindexUV + 1) : "") << "/" << (hasNrm ? std::to_string(centerindexNormal + 1) : "") << " ";
					outFile << curVertices[l - 1] + 1 << "/" << (numUVs >= 1 ? std::to_string(curUvs[l - 1] + 1) : "") << "/" << (hasNrm ? std::to_string(curNormals[l - 1] + 1) : "") << " ";
					outFile << curVertices[l] + 1 << "/" << (numUVs >= 1 ? std::to_string(curUvs[l] + 1) : "") << "/" << (hasNrm ? std::to_string(curNormals[l] + 1) : "") << " ";
					outFile << std::endl;
				}
				break;
			default: // This could already be the header of the next section...
				std::cout << "Warning: Encountered unknown primitive Flag (" << std::to_string(primitveFlag) << ") in Section: " << i << " at global offset " << std::hex << inputFile.tellg() << std::dec << std::endl;
				inputFile.seekg(-3, inputFile.cur); // Set the file pointer 3 bytes (flags and count) back to we don't get a corrupted header if this was the next section
				bytesRead -= 3;
				primitveFlag = 0;			
				break;
			}
		}
		inputFile.ignore(fileHeader.sectionSizes[i] - bytesRead); // We need to ignore the rest of the section because they are 32 byte aligned...
	}


	// Change Byte Order!
	fileHeader.boundingBox[0].x = FloatSwap(fileHeader.boundingBox[0].x);
	fileHeader.boundingBox[0].y = FloatSwap(fileHeader.boundingBox[0].y);
	fileHeader.boundingBox[0].z = FloatSwap(fileHeader.boundingBox[0].z);
	fileHeader.boundingBox[1].x = FloatSwap(fileHeader.boundingBox[1].x);
	fileHeader.boundingBox[1].y = FloatSwap(fileHeader.boundingBox[1].y);
	fileHeader.boundingBox[1].z = FloatSwap(fileHeader.boundingBox[1].z);

	std::cout << "Done!" << std::endl;
	std::cout << "Min Bounding Box: " << "x: " << fileHeader.boundingBox[0].x << '\t' << "y: " << fileHeader.boundingBox[0].y << '\t' << "z: " << fileHeader.boundingBox[0].z << std::endl;
	std::cout << "Max Bounding Box: " << "x: " << fileHeader.boundingBox[1].x << '\t' << "y: " << fileHeader.boundingBox[1].y << '\t' << "z: " << fileHeader.boundingBox[1].z << std::endl;

	inputFile.close();
	exit(0);
}
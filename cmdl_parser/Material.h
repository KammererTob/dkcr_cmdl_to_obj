#pragma once

#include <string>
#include <fstream>
#include <iostream>
#include <stdint.h>
#include <intrin.h>
#include <sstream>
#include <vector>
#include <memory>

class Material
{
public:
	Material(const std::string materialName);
	Material(uint32_t vertexAttributeFlags, uint64_t textureId);
	virtual ~Material();

	void convertTXTRtoDDS(const std::string &textureDir = "");
	void setTextureId(uint64_t textureId);
	uint64_t getTextureId() const;
	void setVertexAttributeFlags(uint32_t flags);
	uint32_t getVertexAttributeFlags() const;
	std::string getMaterialName() const;
	std::string getMaterialDefinition() const;

	template <typename T>
	void addMaterialSection(Material &material, char* buffer, int size);

private:
	void uncompressBytesAndWrite(std::vector<uint8_t> &byteArray, int width, int height, std::ofstream &outFile);
	unsigned char reverseBits(unsigned char b);
	unsigned char swapBits(unsigned char b);

private:
	uint32_t vertexAttributeFlags;
	uint64_t textureId;
	std::string materialName;
	std::stringstream materialDefinition;

};

template <typename T>
void Material::addMaterialSection(Material &material, char* buffer, int size)
{
	T sectionObj = T();
	this->materialDefinition << sectionObj.parseSection(material, buffer, size);
}

class SectionType
{
public:
	virtual std::string parseSection(Material &material, char* buffer, int size);
};

class Pass : public SectionType
{
public:
	std::string parseSection(Material &material, char* buffer, int size);
};

class Clr : public SectionType
{
public:
	std::string parseSection(Material &material, char* buffer, int size);
};

class Int : public SectionType
{
public:
	std::string parseSection(Material &material, char* buffer, int size);
};


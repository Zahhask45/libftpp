#include "data_buffer.hpp"
#include <cstring>

void DataBuffer::serialize(const std::string &str){
	size_t size = str.size();
	serialize(size);

	const std::byte *bytes = reinterpret_cast<const std::byte*>(str.data());
	const std::byte *bytesEnd = reinterpret_cast<const std::byte*>(str.data() + size);
	buffer.insert(buffer.end(), bytes, bytesEnd);
}

void DataBuffer::deserialize(std::string &str){
	size_t size;

	if (buffer.size() < sizeof(size))
		throw NotEnoughBytesException();

	std::memcpy(&size, buffer.data(), sizeof(size));

	if (buffer.size() - sizeof(size) < size)
		throw NotEnoughBytesException();

	const char *bytes = reinterpret_cast<const char*>(buffer.data() + sizeof(size));
	str.assign(bytes, size);
	buffer.erase(buffer.begin(), buffer.begin() + sizeof(size) + size);
}

std::vector<std::byte> &DataBuffer::data() noexcept{
	return buffer;
}

const std::vector<std::byte> &DataBuffer::data() const noexcept{
	return buffer;
}

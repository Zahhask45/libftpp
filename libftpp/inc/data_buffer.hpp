#ifndef DATA_BUFFER_HPP
#define DATA_BUFFER_HPP

#include <vector>
#include <type_traits>
#include <cstddef>
#include <cstring>
#include <string>
#include <stdexcept>

class DataBuffer{
	public:
		template <typename TType>
		friend DataBuffer& operator<<(DataBuffer& data, const TType& obj){
			data.serialize(obj);
			return data;
		};
		
		template <typename TType>
		friend DataBuffer& operator>>(DataBuffer& data, TType& obj){
			data.deserialize(obj);
			return data;
		};

		std::vector<std::byte>& data() noexcept;
		const std::vector<std::byte>& data() const noexcept;
	
	public:
		class NotEnoughBytesException : public std::runtime_error {
			public:
				explicit NotEnoughBytesException():
				runtime_error("DataBuffer: Not enough bytes to deserialize"){}
		};
		
	private:
		std::vector<std::byte> buffer;

		template <typename TType>
		std::enable_if_t<std::is_trivially_copyable_v<TType>, void> serialize(const TType &obj){
			const std::byte *bytes = reinterpret_cast<const std::byte*>(&obj);
			buffer.insert(buffer.end(), bytes, bytes + sizeof(TType));
		};

		void serialize(const std::string &str);

		template <typename TType>
		std::enable_if_t<std::is_trivially_copyable_v<TType>, void> deserialize(TType &obj){
			if (buffer.size() < sizeof(TType))
				throw NotEnoughBytesException();

			std::memcpy(&obj, buffer.data(), sizeof(TType));
			buffer.erase(buffer.begin(), buffer.begin() + sizeof(TType));
		};
		
		void deserialize(std::string &str);
};

#endif


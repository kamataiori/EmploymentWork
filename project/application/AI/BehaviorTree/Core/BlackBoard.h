#pragma once
#include <unordered_map>
#include <string>
#include <any>
#include <stdexcept>

//======================================================
// BlackBoard
// ノード間共有データ
//======================================================
class BlackBoard {
public:
    template<typename T>
    void set_value(const std::string& key, const T& value)
    {
        mData[key] = value;
    }

    template<typename T>
    T get_value(const std::string& key) const
    {
        auto it = mData.find(key);
        if (it == mData.end()) {
            throw std::runtime_error("BlackBoard key not found: " + key);
        }

        try {
            return std::any_cast<T>(it->second);
        }
        catch (const std::bad_any_cast&) {
            throw std::runtime_error("BlackBoard bad_any_cast for key: " + key);
        }
    }

    bool has_key(const std::string& key) const
    {
        return mData.find(key) != mData.end();
    }

private:
    std::unordered_map<std::string, std::any> mData;
};

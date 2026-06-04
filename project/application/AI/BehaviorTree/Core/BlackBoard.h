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

    //==================================================
    // try_get_value
    // ・keyが存在しない/型が違う場合でも例外を投げず false
    // ・成功時は out に値を入れて true
    //==================================================
    template<typename T>
    bool try_get_value(const std::string& key, T& out) const
    {
        auto it = mData.find(key);
        if (it == mData.end()) {
            return false;
        }

        // any_cast<T>(&) なら例外を投げずに nullptr / pointer で判定できる
        const T* p = std::any_cast<T>(&(it->second));
        if (!p) {
            return false;
        }

        out = *p;
        return true;
    }

    bool has_key(const std::string& key) const
    {
        return mData.find(key) != mData.end();
    }

private:
    std::unordered_map<std::string, std::any> mData;
};
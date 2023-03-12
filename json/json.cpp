#include <cctype>
#include <new>
#include <sstream>
#include <vector>
#include <map>
#include <string>
#include <initializer_list>
#include <fstream>
#include <cstring>
#include <cassert>

#include "json.h"

#include <cstdio>
#define dmsg(fmt, ...) if (g_enable_json_dbg) fprintf(stdout, "[%s:%s:%d] " fmt "\n", __FILE__, __func__, __LINE__, ##__VA_ARGS__)
static bool g_enable_json_dbg = false;

class json {
    public:
        typedef enum { BOL, NUM, STR, ARR, OBJ, NUL, } type_t;
        typedef bool bol_t;
        typedef double num_t;
        typedef std::string str_t;
        typedef std::vector<json *> arr_t;
        typedef std::map<str_t, json *> obj_t;

    public:
        json(type_t type):
            m_type(type), m_key("")
    {
        switch (type) {
            case BOL: m_bol = bol_t(); break;
            case NUM: m_num = num_t(); break;
            case STR: m_str = new(std::nothrow) str_t(); break;
            case ARR: m_arr = new(std::nothrow) arr_t(); break;
            case OBJ: m_obj = new(std::nothrow) obj_t(); break;
            case NUL: break;
            default: break;
        }
    }

        json(const str_t &key): 
            m_type(NUL), m_key(key) 
    {}

        json(const str_t &key, const bool        &bol): 
            m_type(BOL), m_key(key), m_bol(bol) 
    {}

        json(const str_t &key, const double      &num): 
            m_type(NUM), m_key(key), m_num(num) 
    {}

        json(const str_t &key, const int         &num): 
            m_type(NUM), m_key(key), m_num(num) 
    {}

        json(const str_t &key, const str_t &str): 
            m_type(STR), m_key(key), m_str(new(std::nothrow) str_t(str)) 
    {}

        json(const str_t &key, const std::initializer_list<double> &li):
            m_type(ARR), m_key(key), m_arr(new(std::nothrow) arr_t)
    { 
        for (const auto &num: li) m_arr->push_back(new(std::nothrow) json("", num)); 
    }

        json(const str_t &key, const std::initializer_list<int> &li):
            m_type(ARR), m_key(key), m_arr(new(std::nothrow) arr_t)
    { 
        for (const auto &num: li) m_arr->push_back(new(std::nothrow) json("", num));
    }

        json(const str_t &key, const std::initializer_list<const str_t> &li):
            m_type(ARR), m_key(key), m_arr(new(std::nothrow) arr_t)
    { 
        for (const auto &str: li) m_arr->push_back(new(std::nothrow) json("", str)); 
    }

        json(const str_t &key, const std::initializer_list<json *> &li):
            m_type(OBJ), m_key(key), m_obj(new(std::nothrow) obj_t)
    {
        for (const auto &obj: li) (*m_obj)[obj->m_key] = obj; 
    }

        ~json()
        {
            switch (m_type) {
                case STR: del_str(); break;
                case ARR: del_arr(); break;
                case OBJ: del_obj(); break;
                default: break;
            }
        }

        json &operator=(const bol_t &bol)
        {
            if (set_type(BOL)) {
                throw json("fail to set type bol");
                return *this;
            }

            m_bol = bol;

            return *this;
        }

        json &operator=(const int &num)
        {
            return operator=(num_t(num));
        }

        json &operator=(const num_t &num)
        {
            if (set_type(NUM)) {
                throw json("fail to set type num");
                return *this;
            }
            m_num = num;

            return *this;
        }

        json &operator=(const char *str)
        {
            return operator=(str_t(str));
        }

        json &operator=(const str_t &str)
        {
            if (set_type(STR)) {
                throw json("fail to set type str");
                return *this;
            }
            *m_str = str;

            return *this;
        }

        json &operator=(const arr_t &arr)
        {
            if (set_type(ARR)) {
                throw json("fial to set type arr");
                return *this;
            }

            for (const auto &iter: arr) {
                json *dup_js = dup_json(iter);
                if (dup_js) {
                    throw json("fail to dup arr");
                    return *this;
                }
                m_arr->push_back(dup_js);
            }

            return *this;
        }

        json &operator=(const json &js)
        {
            std::string err;

            del_type();

            switch (js.m_type) {
                case NUM: 
                    m_type = NUM;
                    m_num = js.m_num; 
                    break;
                case BOL: 
                    m_type = BOL;
                    m_bol = js.m_bol; 
                    break;
                case STR: 
                    m_type = STR;
                    m_str = new(std::nothrow) str_t(*js.m_str);
                    if (!m_str)
                        throw json("fail to dup str");
                    break;
                case ARR:
                    m_type = ARR;
                    m_arr = dup_arr(js.m_arr);
                    if (!m_arr)
                        throw json("fail to dup arr");
                    break;
                case OBJ:
                    m_type = OBJ;
                    m_obj = dup_obj(js.m_obj);
                    if (!m_obj)
                        throw json("fail to dup obj");
                    break;
                case NUL:
                default:
                    m_type = NUL;
                    break;
            }

            return *this;
        }

        json &operator=(const obj_t &obj)
        {
            if (set_type(STR)) {
                throw json("fail to set type obj");
                return *this;
            }

            for (const auto &iter: obj) {
                json *dup_js = dup_json(iter.second);
                if (dup_js) {
                    throw json("fail to dup obj");
                    return *this;
                }
                (*m_obj)[iter.first] = dup_js;
            }

            return *this;
        }

        json &operator[](const str_t &key)
        { 
            if (OBJ != m_type) {
                throw json("not obj");
                return *this;
            } 

            auto iter = m_obj->find(key);
            if (iter != m_obj->end())
                return *iter->second;

            json *add = new(std::nothrow) json(key);
            if (!add) {
                throw json(std::string("fail to add key: ") + key);
                return *this;
            }
            (*m_obj)[key] = add;
            return *add;
        }

        type_t type()
        { 
            return m_type; 
        }

        void set_key(const str_t &key)
        { 
            m_key = key; 
        }

        str_t key()
        {
            return m_key;
        }

        bool bol()
        { 
            return BOL == m_type ? m_bol : false; 
        }

        double num()
        { 
            return NUM == m_type ? m_num : 0; 
        }

        str_t *str()
        {
            return STR == m_type ? m_str : nullptr; 
        }

        arr_t *arr()
        {
            return ARR == m_type ? m_arr : nullptr;
        }

        obj_t *obj()
        { 
            return OBJ == m_type ? m_obj : nullptr; 
        }

        bool is_num(const str_t &str) 
        {
            for (const auto &ch: str)
                if (!std::isdigit(ch) && '.' != ch)
                    return false;
            return true;
        }

        bool is_bol(const str_t &str) 
        {
            return (((str.length() == (sizeof("true")-1)) && 
                        !strcasecmp("true", str.c_str())) ||
                    ((str.length() == (sizeof("false")-1)) &&
                     !strcasecmp("false", str.c_str())));
        }

        void del_type()
        {
            switch (m_type) {
                case STR: del_str(); break;
                case ARR: del_arr(); break;
                case OBJ: del_obj(); break;
                case BOL: m_bol = false; break;
                case NUM: m_num = 0; break;
                case NUL:
                default: m_type = NUL; break;
            }
        }

        void del_str()
        {
            assert(STR == m_type);
            delete m_str;
            m_str = nullptr;
            m_type = NUL;
        }

        void del_arr()
        {
            assert(ARR == m_type);
            for (auto &iter: *m_arr) {
                if (!iter) {
                    continue;
                }

                //dmsg("del: %s\n", (iter)->to_str().c_str());

                delete iter;
            } 
            m_arr->clear();
            delete m_arr;
            m_arr = nullptr;
            m_type = NUL;
        }

        void del_obj()
        {
            assert(OBJ == m_type);
            for (auto &obj: *m_obj) {
                if (!obj.second) {
                    continue;
                }
                // dmsg("del: %s\n", obj->to_str().c_str());

                delete obj.second;
            }
            m_obj->clear();
            delete m_obj;
            m_obj = nullptr;
            m_type = NUL;
        }

        int set_type(type_t type)
        {
            if (type == m_type)
                return 0;

            del_type();

            switch (type) {
                case BOL: 
                    m_bol = bol_t(); 
                    break;
                case NUM: 
                    m_num = num_t();
                    break;
                case STR: 
                    m_str = new(std::nothrow) str_t(); 
                    if (!m_str)
                        return -1;
                    break;
                case ARR: 
                    m_arr = new(std::nothrow) arr_t(); 
                    if (!m_arr)
                        return -1;
                    break;
                case OBJ:
                    m_obj = new(std::nothrow) obj_t();
                    if (!m_obj)
                        return -1;
                    break;
                case NUL: 
                default: 
                    break;
            }

            m_type = type;

            return 0;
        }

        str_t to_str();
        int to_file(const char *file_name);
        static json *load_str(const str_t &str);
        static json *load_file(const char *file_name);

    private:
        static json *load_str_arr(const str_t &str, size_t &offset);
        static json *load_str_obj(const str_t &str, size_t &offset);
        static json *load_str_key_val(const str_t &str, size_t &offset);
        static json *dup_json(const json *js);
        static arr_t *dup_arr(const arr_t *arr);
        static obj_t *dup_obj(const obj_t *obj);

    private:
        type_t m_type;
        str_t m_key;
        union {
            bol_t m_bol;
            num_t m_num;
            str_t *m_str;
            arr_t *m_arr;
            obj_t *m_obj;
        };
};

json::str_t json::to_str()
{
    str_t out_str;
    bool is_first = true;

    if (!m_key.empty())
        out_str = "\"" + m_key + "\": ";

    switch (m_type) {
        case NUL:
            out_str += "null";
            break;
        case NUM:
            out_str += std::to_string(num());
            break;
        case STR:
            out_str += '"' + *str() + '"';
            break;
        case BOL:
            out_str += (bol() ? "true" : "false");
            break;
        case ARR:
            out_str += "[ ";
            if (!arr()) {
                out_str += " ]";
                break;
            }
            for (const auto &iter: *arr()) {
                if (!iter)
                    continue;
                if (!is_first)
                    out_str += ", ";
                out_str += iter->to_str();
                is_first = false;
            }
            out_str += " ]";
            break;
        case OBJ:
            dmsg("");
            out_str += "{ ";
            if (!obj()) {
                out_str += " }";
                break;
            }
            for (const auto &iter: *obj()) {
                if (!iter.second)
                    continue;
                if (!is_first)
                    out_str += ", ";
                out_str += iter.second->to_str();
                is_first = false;
            }
            out_str += " }";
            break;
        default:
            break;
    }

    return out_str;
}

json *json::load_str_key_val(const str_t &str, size_t &offset)
{
    std::string key;
    std::string val;
    bool bol = false;
    json::type_t type;
    double num;

    if ('"' != str.at(offset))
        return nullptr;
    ++offset;
    dmsg("");

    for (; offset < str.length(); ++offset) {
        if ('"' == str.at(offset))
            break;
        key += str.at(offset);
    }
    dmsg("");
    if ('"' != str.at(offset))
        return nullptr;
    // skip "
    ++offset;

    // skip :
    ++offset;

    dmsg("");
    if (key.empty())
        return nullptr;
    dmsg("");
    if (offset >= str.length())
        return nullptr;
    dmsg("[->%c", str.at(offset));
    if ('[' == str.at(offset)) {
        json *arr = load_str_arr(str, offset);
        if (!arr)
            return nullptr;
        arr->set_key(key);
        return arr;
    }
    else if ('{' == str.at(offset)) {
        json *obj = load_str_obj(str, offset);
        if (!obj)
            return nullptr;
        obj->set_key(key);
        return obj;
    }

    dmsg("");
    for (; offset < str.length(); ++offset) {
        if (',' == str.at(offset) ||
                ']' == str.at(offset) ||
                '}' == str.at(offset))
            break;
        val += str.at(offset);
        dmsg("");
    }
    if (val.empty())
        return nullptr;

    dmsg("k:%s, v:%s", key.c_str(), val.c_str());
    if (val.length() > 2 &&
            '"' == *val.begin() &&
            '"' == *(val.end()-1)) {
        dmsg("");
        type = json::type_t::STR;
        val = val.substr(1, val.length()-2);
        dmsg("set val:%s", val.c_str());
    } else if ((val.length() == (sizeof("true")-1)) &&
            !strcasecmp(val.c_str(), "true")) {
        dmsg("");
        type = json::type_t::BOL;
        bol = true;
    } else if ((val.length() == (sizeof("false")-1)) &&
            !strcasecmp(val.c_str(), "false")) {
        dmsg("");
        type = json::type_t::BOL;
        bol = false;
    } else if ((val.length() == (sizeof("null")-1)) &&
            !strcasecmp(val.c_str(), "null")) {
        dmsg("");
        type = json::type_t::NUL;
        (void)val;
    } else {
        dmsg("");
        for (const auto &ch: val)
            if (!std::isdigit(ch) && '.' != ch)
                return nullptr;
        type = json::type_t::NUM;
        num = std::stod(val);
    }

    dmsg("type:%d", type);
    json *js = nullptr;
    switch (type) {
        case json::type_t::NUL:
            js = new(std::nothrow) json(key);
            if (!js)
                return nullptr;
            break;
        case json::type_t::NUM:
            js = new(std::nothrow) json(key, num);
            if (!js)
                return nullptr;
            break;
        case json::type_t::STR:
            dmsg("");
            js = new(std::nothrow) json(key, val);
            if (!js)
                return nullptr;
            break;
        case json::type_t::BOL:
            js = new(std::nothrow) json(key, bol);
            if (!js)
                return nullptr;
            break;
        default:
            return nullptr;
    }
    dmsg("create kv:%s", js->to_str().c_str());

    return js;
}

json *json::load_str_arr(const str_t &str, size_t &offset)
{
    json *arr = nullptr;
    dmsg("arr:instr:%c", str.at(offset));
    if ('[' != str.at(offset)) 
        return nullptr;
    dmsg("");

    // skip [
    ++offset;
    arr = new(std::nothrow) json(json::type_t::ARR);
    if (!arr)
        return nullptr;

    for (; offset < str.length();) {
        if (']' == str.at(offset)) {
            ++offset;
            break;
        } 
        json *iter = nullptr;
        // parser json obj
        if ('{' == str.at(offset)) {
            dmsg("");
            iter = load_str_obj(str, offset);
            if (!iter)
                goto err;
        }
        // parser json arr
        else if ('[' == str.at(offset)) {
            dmsg("");
            iter = load_str_arr(str, offset);
            if (!iter)
                goto err;
        }
        // parser json key
        else if ('"' == str.at(offset)) {
            dmsg("");
            // skip "
            ++offset;
            for (size_t check = offset; check < str.length(); ++check) {
                dmsg("offset:%zu, str:%s", offset, str.substr(offset).c_str());
                if ('"' != str.at(check))
                    continue;
                size_t str_len = check - offset;
                iter = new(std::nothrow) json(str.substr(offset, str_len));
                if (!iter)
                    goto err;
                offset += str_len;
                //skip "
                ++offset;
                break;
            }
        }
        // parser json num
        else if (std::isdigit(str.at(offset))) {
            dmsg("");
            for (size_t check = offset; check < str.length();++check) {
                dmsg("offset:%zu, str:%s", offset, str.substr(offset).c_str());
                if (!std::isdigit(str.at(check)) && 
                        ('.' != str.at(check)) &&
                        (',' != str.at(check)) &&
                        (']' != str.at(check))) {
                    goto err;
                }
                if (',' != str.at(check) &&
                        ']' != str.at(check)) {
                    continue;
                }
                size_t num_len = check - offset;
                if (!num_len)
                    goto err;
                dmsg("offset:%zu, str:%s", offset, str.substr(offset).c_str());
                iter = new(std::nothrow) json("", std::stod(str.substr(offset, num_len)));
                if (!iter)
                    goto err;
                offset += num_len;
                break;
            }
        }
        // parser json bol
        else {
            dmsg("");
            size_t less_str = str.length() - offset;
            if ((less_str > sizeof("true") - 1) &&
                    !strcasecmp("true", str.substr(offset, sizeof("true")-1).c_str())) {
                dmsg("offset:%zu, str:%s", offset, str.substr(offset).c_str());
                iter = new(std::nothrow) json("", true);
                if (!iter)
                    goto err;
                offset += sizeof("true")-1;
            }
            else if ((less_str > sizeof("false") - 1) &&
                    !strcasecmp("false", str.substr(offset, sizeof("fasle")-1).c_str())) {
                dmsg("offset:%zu, str:%s", offset, str.substr(offset).c_str());
                iter = new(std::nothrow) json("", false);
                if (!iter)
                    goto err;
                offset += sizeof("false")-1;
            }
        }
        dmsg("bol done");

        if (!iter)
            goto err;
        dmsg("");
        if (!arr->arr()->empty() &&
                (*arr->arr()->begin())->type() != iter->type()) 
            goto err;
        dmsg("");
        arr->arr()->push_back(iter);

        if (',' == str.at(offset)) {
            ++offset;
            continue;
        }
        if (']' == str.at(offset)) {
            ++offset;
            break;
        }
        dmsg("offset:%zu, str:%s", offset, str.substr(offset).c_str());

        goto err;
    }
    dmsg("");
    return arr;
err:
    dmsg("err");
    if (arr)
        delete arr;
    return nullptr;
}

json *json::load_str_obj(const str_t &str, size_t &offset)
{
    if ('{' != str.at(offset))
        return nullptr;
    // skip {
    ++offset;
    dmsg("str:%s", str.c_str());

    json *obj = new(std::nothrow) json(json::type_t::OBJ);//json("", new json(""));//json::type_t::OBJ);
    if (!obj)
        return nullptr;

    dmsg("");

    for (; offset < str.length();) {
        if ('}' == str.at(offset)) 
            break;
        dmsg("");
        dmsg("str:%s", str.c_str());
        json *iter = nullptr;
        if ('"' == str.at(offset)) {
            dmsg("");
            iter = load_str_key_val(str, offset);
            if (!iter) 
                goto err;
            dmsg("");
        }
        dmsg("");
        if (!iter)
            goto err;
        dmsg("insert iter:%s, type:%d", iter->to_str().c_str(), iter->type());
        json::obj_t *o = obj->obj();
        if (!o)
            goto err;
        assert(iter);
        assert(o);
        (*o)[iter->key()] = iter;
        if (',' == str.at(offset)) {
            ++offset;
            continue;
        }
        dmsg("");
    }
    dmsg("");
    if ('}' != str.at(offset)) {
        goto err;
    }
    // skip }
    ++offset;
    dmsg("");

    return obj;
err:
    if (obj)
        delete obj;
    return nullptr;
}

json *json::load_str(const str_t &str)
{
    str_t parser;
    bool left = false;
    bool right = false;

    dmsg("");

    for (const auto &ch: str) {
        if (left && '"' == ch)
            right = true;
        if (!right && '"' == ch)
            left = true;

        if (left || (!left && ' ' != ch))
            parser += ch;

        if (left && right) {
            left = false;
            right = false;
        }
    }
    if (parser.empty())
        return nullptr;

    dmsg("parser: %s", parser.c_str());

    size_t offset = 0;
    if ('{' == *parser.begin())
        return load_str_obj(parser, offset);
    else if ('[' == *parser.begin())
        return load_str_obj(parser, offset);

    return nullptr;
}

int json::to_file(const char *file_name)
{
    if (!file_name)
        return -1;

    std::ofstream outfile(file_name, std::ios::out | std::ios::trunc );

    outfile << to_str();

    outfile.close();

    return 0;
}

json *json::load_file(const char *file_name)
{
    if (!file_name)
        return nullptr;

    std::ifstream infile(file_name, std::ios::in);

    std::stringstream instream;
    instream << infile.rdbuf();
    str_t instr(instream.str());

    infile.close();

    dmsg("instr: %s\n", instr.c_str());

    return json::load_str(instr);
}

json *json::dup_json(const json *js)
{
    switch (js->m_type) {
        case NUM: return new(std::nothrow) json(js->m_key, js->m_num);
        case BOL: return new(std::nothrow) json(js->m_key, js->m_bol);
        case STR: return new(std::nothrow) json(js->m_key, js->m_str);
        case ARR: return new(std::nothrow) json(js->m_key, dup_arr(js->m_arr));
        case OBJ: return new(std::nothrow) json(js->m_key, dup_obj(js->m_obj));
        case NUL:
        default:  return new(std::nothrow) json(js->m_key);
    }
}

json::arr_t *json::dup_arr(const arr_t *arr)
{
    assert(arr);

    arr_t *new_arr = new(std::nothrow) arr_t;
    if (!new_arr)
        return nullptr;

    for (const auto &iter: *arr) {
        json *dup_iter = dup_json(iter);
        if (!dup_iter)
            goto err;

        new_arr->push_back(dup_iter);
    }

    return new_arr;

err:
    if (new_arr)
        delete new_arr;
    return nullptr;
}

json::obj_t *json::dup_obj(const obj_t *obj)
{
    assert(obj);

    obj_t *new_obj = new(std::nothrow) obj_t;
    if (!new_obj)
        return nullptr;

    for (const auto &iter: *obj) {
        json *dup_iter = dup_json(iter.second);
        if (!dup_iter)
            goto err;

        (*new_obj)[dup_iter->key()] = dup_iter;
    }

    return new_obj;

err:
    if (new_obj)
        delete new_obj;
    return nullptr;
}

#ifndef main

#include <iostream>

using namespace std;

int main(int argc, char *argv[])
{
    (void)argc;

    (void)argv;
    const char *input = argv[1];

    json jnul("jnul");
    json js("js", 100);
    json jstr("jstr", "hi!");
    json jbol("jbol", true);

    json jarr_str("", { "1" , "2", "3"});
    json jobj("", { new json("A", 90), new json("B", false)});
    json jarr_num("", { 1, 2, 3});
    json root("", { 
            new json("A", { 
                    new json("a", 1),
                    new json("b", 2)  
                    }),          
            new json("B", { 1, 2, 3}),
            new json("C")
            });

    cout << "before: " << jobj.to_str() << "\n";
    try {
        jobj["fuck"] = true;
        jobj["moe"] = "?";
        jobj["*"] = 996.007;
        jobj["-"] = 0;
        jobj["num"] = jobj["*"].num() + 1000;
    } catch (json e) {
        cout << "catch fail: " << e.to_str() << "\n";
    }
    cout << "after:  " << jobj.to_str() << "\n";

    cout << js.to_str() << "\n"
        << jstr.to_str() << "\n" 
        << jbol.to_str() << "\n"
        << jobj.to_str() << "\n"
        << jarr_str.to_str() << "\n"
        << jarr_num.to_str() << "\n"
        << root.to_str() << "\n"
        << root["B"].to_str() << "\n"
        << "\n";

    cout << "out > 1.js" << "\n";
    root.to_file("1.js");

    if (input) {
        cout << "read input file name: " << input << "\n";

        json *js = json::load_file(input);
        if (!js)
            return -1;

        cout << "read " << input << ": " << "\n" 
            << js->to_str() << "\n";
        delete js;
    }

    return 0;
}

#endif

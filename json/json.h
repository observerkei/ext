#ifndef __JSON_H__
#define __JSON_H__

#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

#ifdef __cplusplus
}
#endif//__cplusplus


#ifdef __cplusplus

#include <map>
#include <vector>
#include <string>
#include <initializer_list>

class json {
    public:
        typedef enum { BOL, NUM, STR, ARR, OBJ, NUL, } type_t;
        typedef bool bol_t;
        typedef double num_t;
        typedef std::string str_t;
        typedef std::vector<json *> arr_t;
        typedef std::map<str_t, json *> obj_t;

    public:
        json(const type_t &type);
        json(const str_t &key);
        json(const str_t &key, const bol_t       &bol);
        json(const str_t &key, const int         &num);
        json(const str_t &key, const num_t       &num);
        json(const str_t &key, const str_t       &str);
        json(const str_t &key, const std::initializer_list<int>    &li);
        json(const str_t &key, const std::initializer_list<num_t>  &li);
        json(const str_t &key, const std::initializer_list<str_t>  &li);
        json(const str_t &key, const std::initializer_list<json *> &li);
        ~json();

        json *operator=(const bol_t &bol);
        json *operator=(const int &num);
        json *operator=(const num_t &num);
        json *operator=(const char *str);
        json *operator=(const str_t &str);
        json *operator=(const arr_t &arr);
        json *operator=(const obj_t &obj);
        json *operator=(const json &js);
        json *operator[](const str_t &key);

        int set_type(const type_t &type);
        const type_t &type();
        void set_key(const str_t &key);
        const str_t &key();
        int set_bol(const bol_t &bol);
        bol_t bol();
        int set_num(const num_t &num);
        num_t num();
        str_t *str();
        arr_t *arr();
        obj_t *obj();
        const str_t to_str();
        int to_file(const char *file_name);
        static json *load_str(const str_t &str);
        static json *load_file(const char *file_name);

    private:
        bool is_num(const str_t &str);
        bool is_bol(const str_t &str);
        void del_type();
        void del_str();
        void del_arr();
        void del_obj();
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


#endif//__cplusplus

#endif//__JSON_H__

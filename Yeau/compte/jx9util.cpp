#include "jx9util.h"

namespace eau
{
    int jx9_out_callback(const void* msg, unsigned int len, void* data)
    {
        string sztag = "jx9";
        if (data) {
            sztag = (char *)data;
        }
        sztag = "[" + sztag + "]";

        string szmsg = "";
        if (msg && len > 0) {
            szmsg.resize(len+1);
            memcpy((void *)szmsg.data(), msg, len);
        }
    
        cout << sztag << " jx9 msg: " << szmsg << endl;
        return 0;
    }

    long check_jx9_return(unqlite_vm* jx9_vm)
    {
        returnv_assert(jx9_vm, EAU_E_INVALIDARG);

        unqlite_value* jx9_return = NULL;
        int ret = unqlite_vm_config(jx9_vm, UNQLITE_VM_CONFIG_EXEC_VALUE, &jx9_return);
        returnv_assert2(ret, UNQLITE_OK, EAU_E_FAIL);

        ret = unqlite_value_to_int(jx9_return);
        unqlite_vm_release_value(jx9_vm, jx9_return);

        returnv_assert2(ret, UNQLITE_OK, EAU_E_FAIL);        
        return EAU_S_OK;
    }

    long parse_jx9_value(unqlite_value* jx9_array, const string &key, int &value)
    {
        returnv_assert(jx9_array, EAU_E_INVALIDARG);
        returnv_assert(!key.empty(), EAU_E_INVALIDARG);

        unqlite_value* jx9_elem = unqlite_array_fetch(jx9_array, key.c_str(), key.size());
        returnv_assert(jx9_elem, EAU_E_FAIL);
        value = unqlite_value_to_int(jx9_elem);

        return EAU_S_OK;
    }

    long parse_jx9_value(unqlite_value* jx9_array, const string &key, string &value)
    {
        returnv_assert(jx9_array, EAU_E_INVALIDARG);
        returnv_assert(!key.empty(), EAU_E_INVALIDARG);

        unqlite_value* jx9_elem = unqlite_array_fetch(jx9_array, key.c_str(), key.size());
        returnv_assert(jx9_elem, EAU_E_FAIL);
        value = (string)unqlite_value_to_string(jx9_elem, NULL);

        return EAU_S_OK;
    }

    long config_jx9_argv(unqlite_vm* jx9_vm, char *fmt, ...)
    {
        returnv_assert(jx9_vm, EAU_E_INVALIDARG);

        va_list ap;
        char *str = NULL;
        int ret = UNQLITE_OK;

        va_start(ap, fmt);
        while(*fmt) 
        {
            switch(*fmt++){
            case 's':
                str = va_arg(ap, char *);
                ret = unqlite_vm_config(jx9_vm, UNQLITE_VM_CONFIG_ARGV_ENTRY, str);;
                break;
            }
            if (ret != UNQLITE_OK) {
                break;
            }
        }
        va_end(ap);

        returnv_assert2(ret, UNQLITE_OK, EAU_E_FAIL);        
        return EAU_S_OK;
    }
    
    long config_jx9_variable(unqlite_vm* jx9_vm, const vector<pair_t> &ivar)
    {
        returnv_assert(jx9_vm, EAU_E_INVALIDARG);
        returnv_assert(!ivar.empty(), EAU_E_INVALIDARG);

        unqlite_value* jx9_array = unqlite_vm_new_array(jx9_vm);
        returnv_assert(jx9_array, EAU_E_FAIL);
        
        int ret = UNQLITE_OK;
        vector<pair_t>::const_iterator iter;
        for(iter=ivar.begin(); iter != ivar.end(); iter++) {
            unqlite_value* jx9_val = unqlite_vm_new_scalar(jx9_vm);
            ret = unqlite_value_string(jx9_val, iter->second.c_str(), iter->second.size());
            break_assert2(ret, UNQLITE_OK);
            ret = unqlite_array_add_strkey_elem(jx9_array, iter->first.c_str(), jx9_val);
            unqlite_vm_release_value(jx9_vm, jx9_val);
            break_assert2(ret, UNQLITE_OK);
        }

        returnv_assert2(ret, UNQLITE_OK, EAU_E_FAIL);
        ret = unqlite_vm_config(jx9_vm, UNQLITE_VM_CONFIG_CREATE_VAR, kEauRecordInVar, jx9_array);
        unqlite_vm_release_value(jx9_vm, jx9_array);

        returnv_assert2(ret, UNQLITE_OK, EAU_E_FAIL);        
        return EAU_S_OK;
    }

    long config_jx9_output(unqlite_vm* jx9_vm, jx9_out_cb_t pf_out, void *data)
    {
        returnv_assert(jx9_vm, EAU_E_INVALIDARG);

        int ret = unqlite_vm_config(jx9_vm, UNQLITE_VM_CONFIG_OUTPUT, pf_out, data);
        returnv_assert2(ret, UNQLITE_OK, EAU_E_FAIL);        
        return EAU_S_OK;
    }

    long process_jx9_put(unqlite* jx9_db, const char* jx9_prog, const vector<pair_t> &ivar)
    {
        returnv_assert(jx9_db, EAU_E_INVALIDARG);
        returnv_assert(jx9_prog, EAU_E_INVALIDARG);

        long lret = EAU_E_FAIL;
        unqlite_vm* jx9_vm = NULL;

        int ret = unqlite_compile_file(jx9_db, jx9_prog, &jx9_vm);
        returnv_assert2(ret, UNQLITE_OK, EAU_E_FAIL);
        config_jx9_output(jx9_vm, jx9_out_callback, (void *)jx9_prog);

        do {
            break_assert2(config_jx9_variable(jx9_vm, ivar), EAU_S_OK);
            break_assert2(unqlite_vm_exec(jx9_vm), UNQLITE_OK);
            break_assert2(check_jx9_return(jx9_vm), EAU_S_OK);
            lret = EAU_S_OK;
        }while(false);
        unqlite_vm_release(jx9_vm);
        log_assert2(lret, EAU_S_OK);

        return lret;
    }

    long process_jx9_get(unqlite* jx9_db, const char* jx9_prog, const vector<pair_t> &ivar, vector<pair_t> &ovar)
    {
        returnv_assert(jx9_db, EAU_E_INVALIDARG);
        returnv_assert(jx9_prog, EAU_E_INVALIDARG);

        long lret = EAU_E_FAIL;
        unqlite_vm* jx9_vm = NULL;
        unqlite_value* jx9_record = NULL;

        int ret = unqlite_compile_file(jx9_db, jx9_prog, &jx9_vm);
        returnv_assert2(ret, UNQLITE_OK, EAU_E_FAIL);
        config_jx9_output(jx9_vm, jx9_out_callback, (void *)jx9_prog);

        do {
            break_assert2(config_jx9_variable(jx9_vm, ivar), EAU_S_OK);
            break_assert2(unqlite_vm_exec(jx9_vm), UNQLITE_OK);
            break_assert2(check_jx9_return(jx9_vm), EAU_S_OK);

            // extract value from jx9 script
            jx9_record = unqlite_vm_extract_variable(jx9_vm, kEauRecordOutVar);
            break_assert(jx9_record);

            vector<pair_t>::iterator iter;
            for (iter=ovar.begin(); iter != ovar.end(); iter++) {
                break_assert2(parse_jx9_value(jx9_record, iter->first, iter->second), EAU_S_OK);
            }
            lret = EAU_S_OK;
        }while(false);

        unqlite_vm_release_value(jx9_vm, jx9_record);
        unqlite_vm_release(jx9_vm);
        return lret;
    }

} // namespace eau

namespace eau
{
    long new_jx9_json_value(unqlite_vm* jx9_vm, const string &value, unqlite_value* &jx9_value)
    {
        returnv_assert(jx9_vm, EAU_E_FAIL);

        unqlite_value* pvalue = jx9_value;
        if (pvalue == NULL) {
            pvalue = unqlite_vm_new_scalar(jx9_vm);
            returnv_assert(pvalue, EAU_E_FAIL);
        }

        int ret = unqlite_value_string(pvalue, value.c_str(), value.size());
        if(ret == UNQLITE_OK) {
            jx9_value = pvalue;
            return EAU_S_OK;
        }else { 
            if (jx9_value == NULL && pvalue != NULL)
                unqlite_vm_release_value(jx9_vm, pvalue);
            return EAU_E_FAIL;
        }
    }

    // json format: {k:v}, {..., k:v}
    long add_jx9_json_object(unqlite_vm* jx9_vm,
            const pair_ptr_t &key, 
            unqlite_value* &jx9_json)
    {
        returnv_assert(jx9_vm, EAU_E_FAIL);

        long lret = EAU_E_FAIL;
        unqlite_value* pjson = jx9_json;
        if (pjson == NULL) {
            pjson = unqlite_vm_new_array(jx9_vm);
            returnv_assert(pjson, EAU_E_FAIL);
        }

        do {
            unqlite_value* pvalue = (unqlite_value*) key.second;
            int ret = unqlite_array_add_strkey_elem(pjson, key.first.c_str(), pvalue);
            break_assert2(ret, UNQLITE_OK);
            lret = EAU_S_OK;
        }while(false);

        if (lret == EAU_S_OK) {
            jx9_json = pjson;
        }else if (jx9_json == NULL && pjson != NULL) {
            unqlite_vm_release_value(jx9_vm, pjson);
        }
        return lret;
    }

    // json format: {k:v}, {..., k:v}
    long add_jx9_json_object(unqlite_vm* jx9_vm,
            const pair_t &key, 
            unqlite_value* &jx9_json)
    {
        returnv_assert(jx9_vm, EAU_E_FAIL);

        long lret = EAU_E_FAIL;
        unqlite_value* pvalue = NULL;

        do {
            lret = new_jx9_json_value(jx9_vm, key.second, pvalue);
            break_assert2(lret, EAU_S_OK);

            pair_ptr_t key_ptr(key.first, pvalue);
            lret = add_jx9_json_object(jx9_vm, key_ptr, jx9_json);
        }while(false);

        unqlite_vm_release_value(jx9_vm, pvalue);
        return lret;
    }

    // json format:  {k1:v1, k2:v2, ...}
    long add_jx9_json_object(unqlite_vm* jx9_vm,
            const vector<pair_t> &keys, 
            unqlite_value* &jx9_json)
    {
        returnv_assert(jx9_vm, EAU_E_FAIL);

        long lret = EAU_E_FAIL;
        unqlite_value* vjson = NULL;

        vector<pair_t>::const_iterator iter;
        for (iter=keys.begin(); iter != keys.end(); iter++) {
            lret = add_jx9_json_object(jx9_vm, *iter, vjson);
            break_assert(lret == EAU_S_OK);
        }

        if (lret == EAU_S_OK)
            jx9_json = vjson;
        else
            unqlite_vm_release_value(jx9_vm, vjson);
        return lret;
    }

    // json format:  {k1:{k11:v11,k12:v12}, k2:{..}, ...}
    long add_jx9_json_object(unqlite_vm* jx9_vm, 
            const map<string, vector<pair_t> > &keys, 
            unqlite_value* &jx9_json)
    {
        returnv_assert(jx9_vm, EAU_E_FAIL);

        long lret = EAU_E_FAIL;
        unqlite_value* mjson = NULL;
        unqlite_value* vjson = NULL;

        map<string, vector<pair_t> >::const_iterator miter;
        for (miter=keys.begin(); miter != keys.end(); miter++) {
            lret = add_jx9_json_object(jx9_vm, miter->second, vjson);
            break_assert(lret == EAU_S_OK);

            pair_ptr_t kv_mpair(miter->first, vjson);
            lret = add_jx9_json_object(jx9_vm, kv_mpair, mjson);
            break_assert(lret == EAU_S_OK);

            unqlite_vm_release_value(jx9_vm, vjson);
            vjson = NULL;
        }
        unqlite_vm_release_value(jx9_vm, vjson);

        if (lret == EAU_S_OK)
            jx9_json = mjson;
        else
            unqlite_vm_release_value(jx9_vm, mjson);
        return lret;
    }

    long set_jx9_json_variable(unqlite_vm* jx9_vm, const char *jx9_var_name, const unqlite_value *jx9_var)
    {
        returnv_assert(jx9_vm, EAU_E_INVALIDARG);
        returnv_assert(jx9_var_name, EAU_E_INVALIDARG);
        returnv_assert(jx9_var, EAU_E_INVALIDARG);
        
        int ret = unqlite_vm_config(jx9_vm, UNQLITE_VM_CONFIG_CREATE_VAR, jx9_var_name, jx9_var);
        returnv_assert2(ret, UNQLITE_OK, EAU_E_FAIL);        
        return EAU_S_OK;
    }

    long set_jx9_json_output(unqlite_vm* jx9_vm, jx9_out_cb_t pf_out, void *data)
    {
        returnv_assert(jx9_vm, EAU_E_INVALIDARG);

        int ret = unqlite_vm_config(jx9_vm, UNQLITE_VM_CONFIG_OUTPUT, pf_out, data);
        returnv_assert2(ret, UNQLITE_OK, EAU_E_FAIL);        
        return EAU_S_OK;
    }

    long parse_jx9_json_return(unqlite_vm* jx9_vm, int &value)
    {
        returnv_assert(jx9_vm, EAU_E_INVALIDARG);

        unqlite_value* jx9_return = NULL;
        int ret = unqlite_vm_config(jx9_vm, UNQLITE_VM_CONFIG_EXEC_VALUE, &jx9_return);
        returnv_assert2(ret, UNQLITE_OK, EAU_E_FAIL);

        int lret = parse_jx9_value(jx9_return, "ret", value);
        return lret;
    }

    long process_jx9_json_get(unqlite* jx9_db, 
            const char* jx9_prog, 
            const map<string, vector<pair_t> > &ivar, 
            vector<pair_t> &ovar)
    {
        returnv_assert(jx9_db, EAU_E_INVALIDARG);
        returnv_assert(jx9_prog, EAU_E_INVALIDARG);

        long lret = EAU_E_FAIL;
        unqlite_vm* jx9_vm = NULL;

        int ret = unqlite_compile_file(jx9_db, jx9_prog, &jx9_vm);
        returnv_assert2(ret, UNQLITE_OK, EAU_E_FAIL);
        set_jx9_json_output(jx9_vm, jx9_out_callback, (void *)jx9_prog);

        unqlite_value* jx9_json = NULL;
        unqlite_value* jx9_arg = NULL;
        do {
            if (!ivar.empty()) {
                lret = add_jx9_json_object(jx9_vm, ivar, jx9_json);
                break_assert(lret == EAU_S_OK);
                lret = set_jx9_json_variable(jx9_vm, kEauJx9Arg, jx9_json);
                break_assert(lret == EAU_S_OK);
            }

            ret = unqlite_vm_exec(jx9_vm);
            break_assert2(ret, UNQLITE_OK);
            
            int jx9_ret = 0xff;
            lret = parse_jx9_json_return(jx9_vm, jx9_ret);
            break_assert(lret == EAU_S_OK && jx9_ret == 0);

            // extract value from jx9 script
            if(!ovar.empty()) {
                jx9_arg = unqlite_vm_extract_variable(jx9_vm, kEauJx9Out);
                vector<pair_t>::iterator iter;
                for (iter=ovar.begin(); iter != ovar.end(); iter++) {
                    parse_jx9_value(jx9_arg, iter->first, iter->second);
                }
            }
            lret = EAU_S_OK;
        }while(false);

        unqlite_vm_release_value(jx9_vm, jx9_json);
        unqlite_vm_release_value(jx9_vm, jx9_arg);
        return lret;
    }

} // namespace eau


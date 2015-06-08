//
//  echodemo.c
//  SuperNET API extension example plugin
//  crypto777
//
//  Copyright (c) 2015 jl777. All rights reserved.
//
// add to limbo:  txids 204961939803594792, 7301360590217481477, 14387806392702706073 for a total of 568.1248 BTCD
// 17638509709909095430 ~170
// http://chain.explorebtcd.info/tx/1dc0faf122e64aa46393291f00483f7779b73504573c1776301347545b760ea9


#define DEPOSIT_XFER_DURATION 30
#define MIN_DEPOSIT_FACTOR 5

#define BUNDLED
#define PLUGINSTR "MGW"
#define PLUGNAME(NAME) MGW ## NAME
#define STRUCTNAME struct PLUGNAME(_info)
#define STRINGIFY(NAME) #NAME
#define PLUGIN_EXTRASIZE sizeof(STRUCTNAME)

#define DEFINES_ONLY
#include "../plugin777.c"
#include "storage.c"
#include "system777.c"
#include "NXT777.c"
#include "ramchain.c"
#undef DEFINES_ONLY

int32_t MGW_idle(struct plugin_info *plugin)
{
    return(0);
}

STRUCTNAME MGW;
char *PLUGNAME(_methods)[] = { "myacctpubkeys", "msigaddr" };
char *PLUGNAME(_pubmethods)[] = { "myacctpubkeys", "msigaddr" };
char *PLUGNAME(_authmethods)[] = { "myacctpubkeys", "msigaddr" };

uint64_t PLUGNAME(_register)(struct plugin_info *plugin,STRUCTNAME   *data,cJSON *json)
{
    uint64_t disableflags = 0;
    printf("init %s size.%ld\n",plugin->name,sizeof(struct MGW_info));
    return(disableflags); // set bits corresponding to array position in _methods[]
}

int32_t get_NXT_coininfo(uint64_t srvbits,uint64_t nxt64bits,char *coinstr,char *coinaddr,char *pubkey)
{
    uint64_t key[3]; char *keycoinaddr,buf[256]; int32_t flag,len = sizeof(buf);
    key[0] = stringbits(coinstr);
    key[1] = srvbits;
    key[2] = nxt64bits;
    flag = 0;
    coinaddr[0] = pubkey[0] = 0;
    if ( (keycoinaddr= db777_read(buf,&len,0,DB_NXTaccts,key,sizeof(key),0)) != 0 )
    {
        strcpy(coinaddr,keycoinaddr);
        //free(keycoinaddr);
    }
    if ( coinaddr[0] != 0 )
        db777_findstr(pubkey,512,DB_NXTaccts,coinaddr);
//printf("(%llu %llu) get.(%s) -> (%s)\n",(long long)srvbits,(long long)nxt64bits,coinaddr,pubkey);
    return(coinaddr[0] != 0 && pubkey[0] != 0);
}

int32_t add_NXT_coininfo(uint64_t srvbits,uint64_t nxt64bits,char *coinstr,char *newcoinaddr,char *newpubkey)
{
    uint64_t key[3]; char *coinaddr,pubkey[513],buf[1024]; int32_t len = sizeof(buf),flag,updated = 0;
    key[0] = stringbits(coinstr);
    key[1] = srvbits;
    key[2] = nxt64bits;
    flag = 1;
    if ( (coinaddr= db777_read(buf,&len,0,DB_NXTaccts,key,sizeof(key),0)) != 0 )
    {
        if ( strcmp(coinaddr,newcoinaddr) == 0 )
            flag = 0;
    }
    //if ( flag != 0 )
    {
        if ( db777_write(0,DB_NXTaccts,key,sizeof(key),newcoinaddr,(int32_t)strlen(newcoinaddr)+1) == 0 )
            updated = 1;
        else printf("error adding (%s)\n",newcoinaddr);
    }
    flag = 1;
    if ( db777_findstr(pubkey,sizeof(pubkey),DB_NXTaccts,newcoinaddr) > 0 )
    {
        if ( strcmp(pubkey,newpubkey) == 0 )
            flag = 0;
    }
    //printf("(%llu %llu) add.(%s) -> (%s) flag.%d\n",(long long)srvbits,(long long)nxt64bits,newcoinaddr,newpubkey,flag);
    //if ( flag != 0 )
    {
        if ( db777_addstr(DB_NXTaccts,newcoinaddr,newpubkey) == 0 )
            updated = 1;//, printf("added (%s)\n",newpubkey);
        else printf("error adding (%s)\n",newpubkey);
    }
    return(updated);
}

struct multisig_addr *find_msigaddr(struct multisig_addr *msig,int32_t *lenp,char *coinstr,char *multisigaddr)
{
    char keystr[1024];
    sprintf(keystr,"%s.%s",coinstr,multisigaddr);
    printf("search_msig.(%s)\n",keystr);
    return(db777_read(msig,lenp,0,DB_msigs,keystr,(int32_t)strlen(keystr)+1,0));
}

int32_t save_msigaddr(char *coinstr,char *NXTaddr,struct multisig_addr *msig)
{
    char keystr[1024];
    sprintf(keystr,"%s.%s",coinstr,msig->multisigaddr);
    printf("save_msig.(%s)\n",keystr);
    return(db777_write(0,DB_msigs,keystr,(int32_t)strlen(keystr)+1,msig,msig->size));
}

int32_t get_redeemscript(char *redeemScript,char *normaladdr,char *coinstr,char *serverport,char *userpass,char *multisigaddr)
{
    cJSON *json,*array,*json2;
    char args[1024],addr[1024],*retstr,*retstr2;
    int32_t i,n,ismine = 0;
    redeemScript[0] = normaladdr[0] = 0;
    sprintf(args,"\"%s\"",multisigaddr);
    if ( (retstr= bitcoind_passthru(coinstr,serverport,userpass,"validateaddress",args)) != 0 )
    {
        printf("get_redeemscript retstr.(%s)\n",retstr);
        if ( (json= cJSON_Parse(retstr)) != 0 )
        {
            if ( (array= cJSON_GetObjectItem(json,"addresses")) != 0 && is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
            {
                for (i=0; i<n; i++)
                {
                    ismine = 0;
                    copy_cJSON(addr,cJSON_GetArrayItem(array,i));
                    if ( addr[0] != 0 )
                    {
                        sprintf(args,"\"%s\"",addr);
                        retstr2 = bitcoind_passthru(coinstr,serverport,userpass,"validateaddress",args);
                        if ( retstr2 != 0 )
                        {
                            if ( (json2= cJSON_Parse(retstr2)) != 0 )
                                ismine = is_cJSON_True(cJSON_GetObjectItem(json2,"ismine")), free_json(json2);
                            free(retstr2);
                        }
                    }
                    if ( ismine != 0 )
                    {
                        //printf("(%s) ismine.%d\n",addr,ismine);
                        strcpy(normaladdr,addr);
                        copy_cJSON(redeemScript,cJSON_GetObjectItem(json,"hex"));
                        break;
                    }
                }
            } free_json(json);
        } free(retstr);
    }
    return(ismine);
}

int32_t _map_msigaddr(char *redeemScript,char *coinstr,char *serverport,char *userpass,char *normaladdr,char *msigaddr,int32_t gatewayid,int32_t numgateways) //could map to rawind, but this is rarely called
{
    int32_t ismine,len; char buf[8192]; struct multisig_addr *msig;
    redeemScript[0] = normaladdr[0] = 0;
    if ( (msig= find_msigaddr((struct multisig_addr *)buf,&len,coinstr,msigaddr)) == 0 )
    {
        strcpy(normaladdr,msigaddr);
        printf("cant find_msigaddr.(%s)\n",msigaddr);
        return(0);
    }
    if ( msig->redeemScript[0] != 0 && gatewayid >= 0 && gatewayid < numgateways )
    {
        strcpy(normaladdr,msig->pubkeys[gatewayid].coinaddr);
        strcpy(redeemScript,msig->redeemScript);
        printf("_map_msigaddr.(%s) -> return (%s) redeem.(%s)\n",msigaddr,normaladdr,redeemScript);
        return(1);
    }
    ismine = get_redeemscript(redeemScript,normaladdr,coinstr,serverport,userpass,msig->multisigaddr);
    if ( normaladdr[0] != 0 )
        return(1);
    strcpy(normaladdr,msigaddr);
    return(-1);
}

int32_t generate_multisigaddr(char *multisigaddr,char *redeemScript,char *coinstr,char *serverport,char *userpass,int32_t addmultisig,char *params)
{
    char addr[1024],*retstr;
    cJSON *json,*redeemobj,*msigobj;
    int32_t flag = 0;
    if ( addmultisig != 0 )
    {
        if ( (retstr= bitcoind_passthru(coinstr,serverport,userpass,"addmultisigaddress",params)) != 0 )
        {
            strcpy(multisigaddr,retstr);
            free(retstr);
            sprintf(addr,"\"%s\"",multisigaddr);
            if ( (retstr= bitcoind_passthru(coinstr,serverport,userpass,"validateaddress",addr)) != 0 )
            {
                json = cJSON_Parse(retstr);
                if ( json == 0 ) printf("Error before: [%s]\n",cJSON_GetErrorPtr());
                else
                {
                    if ( (redeemobj= cJSON_GetObjectItem(json,"hex")) != 0 )
                    {
                        copy_cJSON(redeemScript,redeemobj);
                        flag = 1;
                    } else printf("missing redeemScript in (%s)\n",retstr);
                    free_json(json);
                }
                free(retstr);
            }
        } else printf("error creating multisig address\n");
    }
    else
    {
        if ( (retstr= bitcoind_passthru(coinstr,serverport,userpass,"createmultisig",params)) != 0 )
        {
            json = cJSON_Parse(retstr);
            if ( json == 0 ) printf("Error before: [%s]\n",cJSON_GetErrorPtr());
            else
            {
                if ( (msigobj= cJSON_GetObjectItem(json,"address")) != 0 )
                {
                    if ( (redeemobj= cJSON_GetObjectItem(json,"redeemScript")) != 0 )
                    {
                        copy_cJSON(multisigaddr,msigobj);
                        copy_cJSON(redeemScript,redeemobj);
                        flag = 1;
                    } else printf("missing redeemScript in (%s)\n",retstr);
                } else printf("multisig missing address in (%s) params.(%s)\n",retstr,params);
                free_json(json);
            }
            free(retstr);
        } else printf("error issuing createmultisig.(%s)\n",params);
    }
    return(flag);
}

char *createmultisig_json_params(struct pubkey_info *pubkeys,int32_t m,int32_t n,char *acctparm)
{
    int32_t i;
    char *paramstr = 0;
    cJSON *array,*mobj,*keys,*key;
    keys = cJSON_CreateArray();
    for (i=0; i<n; i++)
    {
        key = cJSON_CreateString(pubkeys[i].pubkey);
        cJSON_AddItemToArray(keys,key);
    }
    mobj = cJSON_CreateNumber(m);
    array = cJSON_CreateArray();
    if ( array != 0 )
    {
        cJSON_AddItemToArray(array,mobj);
        cJSON_AddItemToArray(array,keys);
        if ( acctparm != 0 )
            cJSON_AddItemToArray(array,cJSON_CreateString(acctparm));
        paramstr = cJSON_Print(array);
        _stripwhite(paramstr,' ');
        free_json(array);
    }
    //printf("createmultisig_json_params.(%s)\n",paramstr);
    return(paramstr);
}

int32_t issue_createmultisig(char *multisigaddr,char *redeemScript,char *coinstr,char *serverport,char *userpass,int32_t use_addmultisig,struct multisig_addr *msig)
{
    int32_t flag = 0;
    char *params;
    params = createmultisig_json_params(msig->pubkeys,msig->m,msig->n,(use_addmultisig != 0) ? msig->NXTaddr : 0);
    flag = 0;
    if ( params != 0 )
    {
        flag = generate_multisigaddr(msig->multisigaddr,msig->redeemScript,coinstr,serverport,userpass,use_addmultisig,params);
        free(params);
    } else printf("error generating msig params\n");
    return(flag);
}

struct multisig_addr *alloc_multisig_addr(char *coinstr,int32_t m,int32_t n,char *NXTaddr,char *userpubkey,char *sender)
{
    struct multisig_addr *msig;
    int32_t size = (int32_t)(sizeof(*msig) + n*sizeof(struct pubkey_info));
    msig = calloc(1,size);
    msig->sig = stringbits("multisig");
    msig->size = size;
    msig->n = n;
    msig->created = (uint32_t)time(NULL);
    if ( sender != 0 && sender[0] != 0 )
        msig->sender = calc_nxt64bits(sender);
    safecopy(msig->coinstr,coinstr,sizeof(msig->coinstr));
    safecopy(msig->NXTaddr,NXTaddr,sizeof(msig->NXTaddr));
    if ( userpubkey != 0 && userpubkey[0] != 0 )
        safecopy(msig->NXTpubkey,userpubkey,sizeof(msig->NXTpubkey));
    msig->m = m;
    return(msig);
}

struct multisig_addr *get_NXT_msigaddr(uint64_t *srv64bits,int32_t m,int32_t n,uint64_t nxt64bits,char *coinstr,char coinaddrs[][256],char pubkeys[][1024],char *userNXTpubkey,int32_t buyNXT)
{
    uint64_t key[16]; char NXTpubkey[128],NXTaddr[64],multisigaddr[128],databuf[8192]; int32_t flag,i,keylen,len; struct coin777 *coin;
    struct multisig_addr *msig = 0;
    //printf("get_NXT_msig %llu (%s)\n",(long long)nxt64bits,coinstr);
    expand_nxt64bits(NXTaddr,nxt64bits);
    set_NXTpubkey(NXTpubkey,NXTaddr);
    if ( NXTpubkey[0] == 0 && userNXTpubkey != 0 && userNXTpubkey[0] != 0 )
        strcpy(NXTpubkey,userNXTpubkey);
    key[0] = stringbits(coinstr);
    for (i=0; i<n; i++)
        key[i+1] = srv64bits[i];
    key[i+1] = nxt64bits;
    keylen = (int32_t)(sizeof(*key) * (i+2));
    len = sizeof(multisigaddr);
    if ( db777_read(multisigaddr,&len,0,DB_msigs,key,keylen,0) != 0 )
    {
        len = sizeof(databuf);
        if ( (msig= find_msigaddr((void *)databuf,&len,coinstr,multisigaddr)) != 0 )
        {
            printf("found msig for NXT.%llu -> (%s)\n",(long long)nxt64bits,msig->multisigaddr);
            return(msig);
        }
    }
    msig = alloc_multisig_addr(coinstr,m,n,NXTaddr,NXTpubkey,0);
    memset(databuf,0,sizeof(databuf)), memcpy(databuf,msig,msig->size), free(msig), msig = (struct multisig_addr *)databuf;
    if ( (coin= coin777_find(coinstr,0)) != 0 )
    {
        if ( buyNXT > 100 )
            buyNXT = 100;
        msig->buyNXT = buyNXT;
        for (i=0; i<msig->n; i++)
        {
            //printf("i.%d n.%d msig->n.%d NXT.(%s) (%s) (%s)\n",i,n,msig->n,msig->NXTaddr,coinaddrs[i],pubkeys[i]);
            strcpy(msig->pubkeys[i].coinaddr,coinaddrs[i]);
            strcpy(msig->pubkeys[i].pubkey,pubkeys[i]);
            msig->pubkeys[i].nxt64bits = srv64bits[i];
        }
        flag = issue_createmultisig(msig->multisigaddr,msig->redeemScript,coinstr,coin->serverport,coin->userpass,coin->mgw.use_addmultisig,msig);
        if ( flag == 0 )
            return(0);
        save_msigaddr(coinstr,NXTaddr,msig);
        if ( db777_write(0,DB_msigs,key,keylen,msig->multisigaddr,(int32_t)strlen(msig->multisigaddr)+1) != 0 )
            printf("error saving msig.(%s)\n",msig->multisigaddr);
    } else printf("cant find coin.(%s)\n",coinstr);
    return(msig);
}

long calc_pubkey_jsontxt(int32_t truncated,char *jsontxt,struct pubkey_info *ptr,char *postfix)
{
    if ( truncated != 0 )
        sprintf(jsontxt,"{\"address\":\"%s\"}%s",ptr->coinaddr,postfix);
    else sprintf(jsontxt,"{\"address\":\"%s\",\"pubkey\":\"%s\",\"srv\":\"%llu\"}%s",ptr->coinaddr,ptr->pubkey,(long long)ptr->nxt64bits,postfix);
    return(strlen(jsontxt));
}

char *create_multisig_jsonstr(struct multisig_addr *msig,int32_t truncated)
{
    long i,len = 0;
    struct coin777 *coin;
    int32_t gatewayid = -1;
    char jsontxt[8192],pubkeyjsontxt[8192],rsacct[64];
    if ( msig != 0 )
    {
        //printf("create_multisig %s\n",msig->coinstr);
        if ( (coin= coin777_find(msig->coinstr,0)) != 0 )
            gatewayid = SUPERNET.gatewayid;
        rsacct[0] = 0;
        conv_rsacctstr(rsacct,calc_nxt64bits(msig->NXTaddr));
        pubkeyjsontxt[0] = 0;
        for (i=0; i<msig->n; i++)
            len += calc_pubkey_jsontxt(truncated,pubkeyjsontxt+strlen(pubkeyjsontxt),&msig->pubkeys[i],(i<(msig->n - 1)) ? ", " : "");
        sprintf(jsontxt,"{%s\"sender\":\"%llu\",\"buyNXT\":%u,\"created\":%u,\"M\":%d,\"N\":%d,\"NXTaddr\":\"%s\",\"NXTpubkey\":\"%s\",\"RS\":\"%s\",\"address\":\"%s\",\"redeemScript\":\"%s\",\"coin\":\"%s\",\"gatewayid\":\"%d\",\"pubkey\":[%s]}",truncated==0?"\"requestType\":\"plugin\",\"plugin\":\"coins\",\"method\":\"setmultisig\",":"",(long long)msig->sender,msig->buyNXT,msig->created,msig->m,msig->n,msig->NXTaddr,msig->NXTpubkey,rsacct,msig->multisigaddr,msig->redeemScript,msig->coinstr,gatewayid,pubkeyjsontxt);
        //if ( (MGW_initdone == 0 && Debuglevel > 2) || MGW_initdone != 0 )
        //    printf("(%s) pubkeys len.%ld msigjsonlen.%ld\n",jsontxt,len,strlen(jsontxt));
        //printf("-> (%s)\n",jsontxt);
        return(clonestr(jsontxt));
    }
    else return(0);
}

int32_t ensure_NXT_msigaddr(char *msigjsonstr,char *coinstr,char *NXTaddr,char *userNXTpubkey,int32_t buyNXT)
{
    char coinaddrs[16][256],pubkeys[16][1024],*str;
    int32_t g,m,retval = 0;
    uint64_t nxt64bits;
    struct multisig_addr *msig;
    msigjsonstr[0] = 0;
    nxt64bits = calc_nxt64bits(NXTaddr);
    for (g=m=0; g<SUPERNET.numgateways; g++)
    {
        //printf("(%llu NXT.%llu) g%d: ",(long long)MGW.srv64bits[g],(long long)nxt64bits,g);
        m += get_NXT_coininfo(MGW.srv64bits[g],nxt64bits,coinstr,coinaddrs[g],pubkeys[g]);
    }
    //printf("m.%d ensure.(%s)\n",m,coinstr);
    if ( m == SUPERNET.numgateways && (msig= get_NXT_msigaddr(MGW.srv64bits,MGW.M,SUPERNET.numgateways,nxt64bits,coinstr,coinaddrs,pubkeys,userNXTpubkey,buyNXT)) != 0 )
    {
        if ( (str= create_multisig_jsonstr(msig,0)) != 0 )
        {
            strcpy(msigjsonstr,str);
            _stripwhite(msigjsonstr,' ');
            //printf("ENSURE.(%s)\n",msigjsonstr);
            retval = 1;
            free(str);
        }
        //free(msig);
    }
    return(retval);
}

int32_t process_acctpubkey(int32_t *havemsigp,cJSON *item,int32_t gatewayid,uint64_t gatewaybits)
{
    uint64_t gbits,nxt64bits; int32_t buyNXT,g,updated;
    char msigjsonstr[MAX_JSON_FIELD],userNXTpubkey[MAX_JSON_FIELD],NXTaddr[MAX_JSON_FIELD],coinaddr[MAX_JSON_FIELD],pubkey[MAX_JSON_FIELD],coinstr[MAX_JSON_FIELD];
    *havemsigp = 0;
    copy_cJSON(coinstr,cJSON_GetObjectItem(item,"coin"));
    copy_cJSON(NXTaddr,cJSON_GetObjectItem(item,"userNXT"));
    copy_cJSON(coinaddr,cJSON_GetObjectItem(item,"coinaddr"));
    copy_cJSON(pubkey,cJSON_GetObjectItem(item,"pubkey"));
    copy_cJSON(userNXTpubkey,cJSON_GetObjectItem(item,"userpubkey"));
    buyNXT = get_API_int(cJSON_GetObjectItem(item,"buyNXT"),0);
    g = get_API_int(cJSON_GetObjectItem(item,"gatewayid"),-1);
    gbits = get_API_nxt64bits(cJSON_GetObjectItem(item,"gatewayNXT"));
    if ( g >= 0 )
    {
        if ( g != gatewayid || (gbits != 0 && gbits != gatewaybits) )
        {
            printf("SKIP: SUPERNET.gatewayid %d g.%d vs gatewayid.%d gbits.%llu vs %llu %s\n",SUPERNET.gatewayid,g,gatewayid,(long long)gbits,(long long)gatewaybits,cJSON_Print(item));
            return(0);
        }
    }
    nxt64bits = calc_nxt64bits(NXTaddr);
    //printf("%s.G%d +(%s %s): ",coinstr,g,coinaddr,pubkey);
    updated = add_NXT_coininfo(gatewaybits,nxt64bits,coinstr,coinaddr,pubkey);
    *havemsigp = ensure_NXT_msigaddr(msigjsonstr,coinstr,NXTaddr,userNXTpubkey,buyNXT);
    return(updated);
}

int32_t process_acctpubkeys(char *retbuf,char *jsonstr,cJSON *json)
{
    cJSON *array; uint64_t gatewaybits; int32_t i,havemsig,n=0,gatewayid,count = 0,updated = 0;
    char coinstr[MAX_JSON_FIELD],gatewayNXT[MAX_JSON_FIELD];
    struct coin777 *coin;
    if ( SUPERNET.gatewayid >= 0 )
    {
        copy_cJSON(coinstr,cJSON_GetObjectItem(json,"coin"));
        gatewayid = get_API_int(cJSON_GetObjectItem(json,"gatewayid"),-1);
        copy_cJSON(gatewayNXT,cJSON_GetObjectItem(json,"gatewayNXT"));
        gatewaybits = calc_nxt64bits(gatewayNXT);
        coin = coin777_find(coinstr,0);
        if ( (array= cJSON_GetObjectItem(json,"pubkeys")) != 0 && is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
        {
            //printf("arraysize.%d\n",n);
            for (i=0; i<n; i++)
            {
                updated += process_acctpubkey(&havemsig,cJSON_GetArrayItem(array,i),gatewayid,gatewaybits);
                count += havemsig;
            }
        }
        sprintf(retbuf,"{\"result\":\"success\",\"coin\":\"%s\",\"updated\":%d,\"total\":%d,\"msigs\":%d}",coinstr,updated,n,count);
        //printf("(%s)\n",retbuf);
    }
    return(updated);
}

cJSON *acctpubkey_json(char *coinstr,char *NXTaddr,int32_t gatewayid)
{
    cJSON *json = cJSON_CreateObject();
    cJSON_AddItemToObject(json,"destplugin",cJSON_CreateString("MGW"));
    cJSON_AddItemToObject(json,"method",cJSON_CreateString("myacctpubkeys"));
    cJSON_AddItemToObject(json,"coin",cJSON_CreateString(coinstr));
    cJSON_AddItemToObject(json,"gatewayNXT",cJSON_CreateString(NXTaddr));
    cJSON_AddItemToObject(json,"gatewayid",cJSON_CreateNumber(gatewayid));
    printf("acctpubkey.(%s)\n",cJSON_Print(json));
    return(json);
}

cJSON *msig_itemjson(char *coinstr,char *account,char *coinaddr,char *pubkey,int32_t allfields)
{
    cJSON *item = cJSON_CreateObject();
    cJSON_AddItemToObject(item,"coin",cJSON_CreateString(coinstr));
    cJSON_AddItemToObject(item,"userNXT",cJSON_CreateString(account));
    cJSON_AddItemToObject(item,"coinaddr",cJSON_CreateString(coinaddr));
    cJSON_AddItemToObject(item,"pubkey",cJSON_CreateString(pubkey));
    if ( allfields != 0 && SUPERNET.gatewayid >= 0 )
    {
        cJSON_AddItemToObject(item,"gatewayNXT",cJSON_CreateString(SUPERNET.NXTADDR));
        cJSON_AddItemToObject(item,"gatewayid",cJSON_CreateNumber(SUPERNET.gatewayid));
    }
    //printf("(%s)\n",cJSON_Print(item));
    return(item);
}

int32_t MGW_publishjson(char *retbuf,cJSON *json)
{
    char *retstr; int32_t retval;
    retstr = cJSON_Print(json);
    _stripwhite(retstr,' ');
    nn_send(MGW.all.socks.both.bus,retstr,(int32_t)strlen(retstr)+1,0);//  nn_publish(retstr,1);
    retval = process_acctpubkeys(retbuf,retstr,json);
    //printf("MGW publish.(%s) -> (%s)\n",retstr,retbuf);
    free(retstr);
    return(retval);
}

void fix_msigaddr(struct coin777 *coin,char *NXTaddr)
{
    int32_t MGW_publishjson(char *retbuf,cJSON *json);
    cJSON *msigjson,*array; char retbuf[1024],coinaddr[MAX_JSON_FIELD],pubkey[MAX_JSON_FIELD];
    if ( SUPERNET.gatewayid >= 0 )
    {
        get_acct_coinaddr(coinaddr,coin->name,coin->serverport,coin->userpass,NXTaddr);
        get_pubkey(pubkey,coin->name,coin->serverport,coin->userpass,coinaddr);
        printf("%s new address.(%s) -> (%s) (%s)\n",coin->name,NXTaddr,coinaddr,pubkey);
        if ( (msigjson= acctpubkey_json(coin->name,SUPERNET.NXTADDR,SUPERNET.gatewayid)) != 0 )
        {
            array = cJSON_CreateArray();
            cJSON_AddItemToArray(array,msig_itemjson(coin->name,NXTaddr,coinaddr,pubkey,1));
            cJSON_AddItemToObject(msigjson,"pubkeys",array);
            //printf("send.(%s)\n",cJSON_Print(msigjson));
            MGW_publishjson(retbuf,msigjson);
            free_json(msigjson);
        }
    }
}

char *get_msig_pubkeys(char *coinstr,char *serverport,char *userpass)
{
    char pubkey[512],NXTaddr[64],account[512],coinaddr[512],*retstr = 0;
    cJSON *json,*item,*array = cJSON_CreateArray();
    uint64_t nxt64bits;
    int32_t i,n;
    if ( (retstr= bitcoind_passthru(coinstr,serverport,userpass,"listreceivedbyaddress","[1, true]")) != 0 )
    {
        //printf("listaccounts.(%s)\n",retstr);
        if ( (json= cJSON_Parse(retstr)) != 0 )
        {
            if ( is_cJSON_Array(json) != 0 && (n= cJSON_GetArraySize(json)) > 0 )
            {
                for (i=0; i<n; i++)
                {
                    item = cJSON_GetArrayItem(json,i);
                    copy_cJSON(account,cJSON_GetObjectItem(item,"account"));
                    if ( is_decimalstr(account) > 0 )
                    {
                        nxt64bits = calc_nxt64bits(account);
                        expand_nxt64bits(NXTaddr,nxt64bits);
                        if ( strcmp(account,NXTaddr) == 0 )
                        {
                            copy_cJSON(coinaddr,cJSON_GetObjectItem(item,"address"));
                            if ( get_pubkey(pubkey,coinstr,serverport,userpass,coinaddr) != 0 )
                                cJSON_AddItemToArray(array,msig_itemjson(coinstr,account,coinaddr,pubkey,1));
                        }
                        else printf("decimal.%d (%s) -> (%s)? ",is_decimalstr(account),account,NXTaddr);
                    }
                }
            }
            free_json(json);
        } else printf("couldnt parse.(%s)\n",retstr);
        free(retstr);
    } else printf("listreceivedbyaddress doesnt return any accounts\n");
    retstr = cJSON_Print(array);
    _stripwhite(retstr,' ');
    return(retstr);
}

char *devMGW_command(char *jsonstr,cJSON *json)
{
    int32_t i,buyNXT; uint64_t nxt64bits; char nxtaddr[64],userNXTpubkey[MAX_JSON_FIELD],msigjsonstr[MAX_JSON_FIELD],NXTaddr[MAX_JSON_FIELD],coinstr[1024]; struct coin777 *coin;
    if ( SUPERNET.gatewayid >= 0 )
    {
        copy_cJSON(NXTaddr,cJSON_GetObjectItem(json,"userNXT"));
        if ( NXTaddr[0] != 0 )
        {
            nxt64bits = conv_acctstr(NXTaddr);
            expand_nxt64bits(nxtaddr,nxt64bits);
        } else nxt64bits = 0;
        //printf("NXTaddr.(%s) %llu\n",nxtaddr,(long long)nxt64bits);
        copy_cJSON(coinstr,cJSON_GetObjectItem(json,"coin"));
        copy_cJSON(userNXTpubkey,cJSON_GetObjectItem(json,"userpubkey"));
        buyNXT = get_API_int(cJSON_GetObjectItem(json,"buyNXT"),0);
        printf("NXTaddr.(%s) %llu %s\n",nxtaddr,(long long)nxt64bits,coinstr);
        if ( nxtaddr[0] != 0 && coinstr != 0 && (coin= coin777_find(coinstr,0)) != 0 )
        {
            for (i=0; i<3; i++)
            {
                if ( ensure_NXT_msigaddr(msigjsonstr,coinstr,nxtaddr,userNXTpubkey,buyNXT) == 0 )
                    fix_msigaddr(coin,nxtaddr), msleep(250);
                else return(clonestr(msigjsonstr));
            }
        }
        sprintf(msigjsonstr,"{\"error\":\"cant find multisig address\",\"coin\":\"%s\",\"userNXT\":\"%s\"}",coinstr!=0?coinstr:"",nxtaddr);
        return(clonestr(msigjsonstr));
    } else return(0);
}

int32_t MGW_publish_acctpubkeys(char *coinstr,char *str)
{
    char retbuf[1024];
    cJSON *json,*array;
    if ( SUPERNET.gatewayid >= 0 && (array= cJSON_Parse(str)) != 0 )
    {
        if ( (json= acctpubkey_json(coinstr,SUPERNET.NXTADDR,SUPERNET.gatewayid)) != 0 )
        {
            cJSON_AddItemToObject(json,"pubkeys",array);
            MGW_publishjson(retbuf,json);
            free_json(json);
            printf("%s processed.(%s) SUPERNET.gatewayid %d %s\n",coinstr,retbuf,SUPERNET.gatewayid,SUPERNET.NXTADDR);
            return(0);
        }
    }
    return(-1);
}

int32_t pubkeycmp(struct pubkey_info *ref,struct pubkey_info *cmp)
{
    if ( strcmp(ref->pubkey,cmp->pubkey) != 0 )
        return(1);
    if ( strcmp(ref->coinaddr,cmp->coinaddr) != 0 )
        return(2);
    if ( ref->nxt64bits != cmp->nxt64bits )
        return(3);
    return(0);
}

int32_t msigcmp(struct multisig_addr *ref,struct multisig_addr *msig)
{
    int32_t i,x;
    if ( ref == 0 )
        return(-1);
    if ( strcmp(ref->multisigaddr,msig->multisigaddr) != 0 || msig->m != ref->m || msig->n != ref->n )
    {
        if ( Debuglevel > 3 )
            printf("A ref.(%s) vs msig.(%s)\n",ref->multisigaddr,msig->multisigaddr);
        return(1);
    }
    if ( strcmp(ref->NXTaddr,msig->NXTaddr) != 0 )
    {
        if ( Debuglevel > 3 )
            printf("B ref.(%s) vs msig.(%s)\n",ref->NXTaddr,msig->NXTaddr);
        return(2);
    }
    if ( strcmp(ref->redeemScript,msig->redeemScript) != 0 )
    {
        if ( Debuglevel > 3 )
            printf("C ref.(%s) vs msig.(%s)\n",ref->redeemScript,msig->redeemScript);
        return(3);
    }
    for (i=0; i<ref->n; i++)
        if ( (x= pubkeycmp(&ref->pubkeys[i],&msig->pubkeys[i])) != 0 )
        {
            if ( Debuglevel > 3 )
            {
                switch ( x )
                {
                    case 1: printf("P.%d pubkey ref.(%s) vs msig.(%s)\n",x,ref->pubkeys[i].pubkey,msig->pubkeys[i].pubkey); break;
                    case 2: printf("P.%d pubkey ref.(%s) vs msig.(%s)\n",x,ref->pubkeys[i].coinaddr,msig->pubkeys[i].coinaddr); break;
                    case 3: printf("P.%d pubkey ref.(%llu) vs msig.(%llu)\n",x,(long long)ref->pubkeys[i].nxt64bits,(long long)msig->pubkeys[i].nxt64bits); break;
                    default: printf("unexpected retval.%d\n",x);
                }
            }
            return(4+i);
        }
    return(0);
}


#define BTC_COINID 1
#define LTC_COINID 2
#define DOGE_COINID 4
#define BTCD_COINID 8
void set_legacy_coinid(char *coinstr,int32_t legacyid)
{
    switch ( legacyid )
    {
        case BTC_COINID: strcpy(coinstr,"BTC"); return;
        case LTC_COINID: strcpy(coinstr,"LTC"); return;
        case DOGE_COINID: strcpy(coinstr,"DOGE"); return;
        case BTCD_COINID: strcpy(coinstr,"BTCD"); return;
    }
}

struct multisig_addr *decode_msigjson(char *NXTaddr,cJSON *obj,char *sender)
{
    int32_t j,M,n;
    char nxtstr[512],coinstr[64],ipaddr[64],numstr[64],NXTpubkey[128];
    struct multisig_addr *msig = 0;
    cJSON *pobj,*redeemobj,*pubkeysobj,*addrobj,*nxtobj,*nameobj,*idobj;
    if ( obj == 0 )
    {
        printf("decode_msigjson cant decode null obj\n");
        return(0);
    }
    nameobj = cJSON_GetObjectItem(obj,"coin");
    copy_cJSON(coinstr,nameobj);
    if ( coinstr[0] == 0 )
    {
        if ( (idobj = cJSON_GetObjectItem(obj,"coinid")) != 0 )
        {
            copy_cJSON(numstr,idobj);
            if ( numstr[0] != 0 )
                set_legacy_coinid(coinstr,atoi(numstr));
        }
    }
    if ( coinstr[0] != 0 )
    {
        addrobj = cJSON_GetObjectItem(obj,"address");
        redeemobj = cJSON_GetObjectItem(obj,"redeemScript");
        pubkeysobj = cJSON_GetObjectItem(obj,"pubkey");
        nxtobj = cJSON_GetObjectItem(obj,"NXTaddr");
        if ( nxtobj != 0 )
        {
            copy_cJSON(nxtstr,nxtobj);
            if ( NXTaddr != 0 && strcmp(nxtstr,NXTaddr) != 0 )
                printf("WARNING: mismatched NXTaddr.%s vs %s\n",nxtstr,NXTaddr);
        }
        //printf("msig.%p %p %p %p\n",msig,addrobj,redeemobj,pubkeysobj);
        if ( nxtstr[0] != 0 && addrobj != 0 && redeemobj != 0 && pubkeysobj != 0 )
        {
            n = cJSON_GetArraySize(pubkeysobj);
            M = (int32_t)get_API_int(cJSON_GetObjectItem(obj,"M"),n-1);
            copy_cJSON(NXTpubkey,cJSON_GetObjectItem(obj,"NXTpubkey"));
            if ( NXTpubkey[0] == 0 )
                set_NXTpubkey(NXTpubkey,nxtstr);
            msig = alloc_multisig_addr(coinstr,M,n,nxtstr,NXTpubkey,sender);
            safecopy(msig->coinstr,coinstr,sizeof(msig->coinstr));
            copy_cJSON(msig->redeemScript,redeemobj);
            copy_cJSON(msig->multisigaddr,addrobj);
            msig->buyNXT = (uint32_t)get_API_int(cJSON_GetObjectItem(obj,"buyNXT"),10);
            for (j=0; j<n; j++)
            {
                pobj = cJSON_GetArrayItem(pubkeysobj,j);
                if ( pobj != 0 )
                {
                    copy_cJSON(msig->pubkeys[j].coinaddr,cJSON_GetObjectItem(pobj,"address"));
                    copy_cJSON(msig->pubkeys[j].pubkey,cJSON_GetObjectItem(pobj,"pubkey"));
                    msig->pubkeys[j].nxt64bits = get_API_nxt64bits(cJSON_GetObjectItem(pobj,"srv"));
                    copy_cJSON(ipaddr,cJSON_GetObjectItem(pobj,"ipaddr"));
                    if ( Debuglevel > 2 )
                        fprintf(stderr,"{(%s) (%s) %llu ip.(%s)}.%d ",msig->pubkeys[j].coinaddr,msig->pubkeys[j].pubkey,(long long)msig->pubkeys[j].nxt64bits,ipaddr,j);
                    //if ( ipaddr[0] == 0 && j < 3 )
                    //   strcpy(ipaddr,Server_ipaddrs[j]);
                    //msig->pubkeys[j].ipbits = calc_ipbits(ipaddr);
                } else { free(msig); msig = 0; }
            }
            //printf("NXT.%s -> (%s)\n",nxtstr,msig->multisigaddr);
            if ( Debuglevel > 3 )
                fprintf(stderr,"for msig.%s\n",msig->multisigaddr);
        } else { printf("%p %p %p\n",addrobj,redeemobj,pubkeysobj); free(msig); msig = 0; }
        //printf("return msig.%p\n",msig);
        return(msig);
    } else fprintf(stderr,"decode msig:  error parsing.(%s)\n",cJSON_Print(obj));
    return(0);
}

void *extract_jsonkey(cJSON *item,void *arg,void *arg2)
{
    char *redeemstr = calloc(1,MAX_JSON_FIELD);
    copy_cJSON(redeemstr,cJSON_GetObjectItem(item,arg));
    return(redeemstr);
}

void *extract_jsonints(cJSON *item,void *arg,void *arg2)
{
    char argstr[MAX_JSON_FIELD],*keystr;
    cJSON *obj0=0,*obj1=0;
    if ( arg != 0 )
        obj0 = cJSON_GetObjectItem(item,arg);
    if ( arg2 != 0 )
        obj1 = cJSON_GetObjectItem(item,arg2);
    if ( obj0 != 0 && obj1 != 0 )
    {
        sprintf(argstr,"%llu.%llu",(long long)get_API_int(obj0,0),(long long)get_API_int(obj1,0));
        keystr = calloc(1,strlen(argstr)+1);
        strcpy(keystr,argstr);
        return(keystr);
    } else return(0);
}

void *extract_jsonmsig(cJSON *item,void *arg,void *arg2)
{
    char sender[MAX_JSON_FIELD];
    copy_cJSON(sender,cJSON_GetObjectItem(item,"sender"));
    return(decode_msigjson(0,item,sender));
}

int32_t jsonmsigcmp(void *ref,void *item) { return(msigcmp(ref,item)); }
int32_t jsonstrcmp(void *ref,void *item) { return(strcmp(ref,item)); }

void set_MGW_fname(char *fname,char *dirname,char *NXTaddr)
{
    if ( NXTaddr == 0 )
        sprintf(fname,"%s/%s/ALL",MGW.PATH,dirname);
    else sprintf(fname,"%s/%s/%s",MGW.PATH,dirname,NXTaddr);
}

void set_MGW_msigfname(char *fname,char *NXTaddr) { set_MGW_fname(fname,"msig",NXTaddr); }
void set_MGW_statusfname(char *fname,char *NXTaddr) { set_MGW_fname(fname,"status",NXTaddr); }
void set_MGW_moneysentfname(char *fname,char *NXTaddr) { set_MGW_fname(fname,"sent",NXTaddr); }
void set_MGW_depositfname(char *fname,char *NXTaddr) { set_MGW_fname(fname,"deposit",NXTaddr); }

void save_MGW_file(char *fname,char *jsonstr)
{
    FILE *fp;
    //char cmd[1024];
    if ( (fp= fopen(os_compatible_path(fname),"wb+")) != 0 )
    {
        fwrite(jsonstr,1,strlen(jsonstr),fp);
        fclose(fp);
        //sprintf(cmd,"chmod +r %s",fname);
        //system(cmd);
        //printf("fname.(%s) cmd.(%s)\n",fname,cmd);
    }
}

void save_MGW_status(char *NXTaddr,char *jsonstr)
{
    char fname[1024];
    set_MGW_statusfname(fname,NXTaddr);
    //printf("save_MGW_status.(%s) -> (%s)\n",NXTaddr,fname);
    save_MGW_file(fname,jsonstr);
}

cJSON *update_MGW_file(FILE **fpp,cJSON **newjsonp,char *fname,char *jsonstr)
{
    FILE *fp;
    long fsize;
    cJSON *json,*newjson;
    char cmd[1024],*str;
    *newjsonp = 0;
    *fpp = 0;
    if ( (newjson= cJSON_Parse(jsonstr)) == 0 )
    {
        printf("update_MGW_files: cant parse.(%s)\n",jsonstr);
        return(0);
    }
    if ( (fp= fopen(os_compatible_path(fname),"rb+")) == 0 )
    {
        fp = fopen(os_compatible_path(fname),"wb+");
        if ( fp != 0 )
        {
            if ( (json = cJSON_CreateArray()) != 0 )
            {
                cJSON_AddItemToArray(json,newjson), newjson = 0;
                str = cJSON_Print(json);
                fprintf(fp,"%s",str);
                free(str);
                free_json(json);
            }
            fclose(fp);
#ifndef WIN32
            sprintf(cmd,"chmod +r %s",fname);
            if ( system(os_compatible_path(cmd)) != 0 )
                printf("update_MGW_file chmod error\n");
#endif
        } else printf("couldnt open (%s)\n",fname);
        if ( newjson != 0 )
            free_json(newjson);
        return(0);
    }
    else
    {
        *fpp = fp;
        fseek(fp,0,SEEK_END);
        fsize = ftell(fp);
        rewind(fp);
        str = calloc(1,fsize);
        if ( fread(str,1,fsize,fp) != fsize )
            printf("error reading %ld from %s\n",fsize,fname);
        json = cJSON_Parse(str);
        free(str);
        *newjsonp = newjson;
        return(json);
    }
}

cJSON *append_MGW_file(char *fname,FILE *fp,cJSON *json,cJSON *newjson)
{
    char *str;
    cJSON_AddItemToArray(json,newjson);//, newjson = 0;
    str = cJSON_Print(json);
    rewind(fp);
    fprintf(fp,"%s",str);
    free(str);
    printf("updated (%s)\n",fname);
    return(0);
}

int32_t update_MGW_jsonfile(void (*setfname)(char *fname,char *NXTaddr),void *(*extract_jsondata)(cJSON *item,void *arg,void *arg2),int32_t (*jsoncmp)(void *ref,void *item),char *NXTaddr,char *jsonstr,void *arg,void *arg2)
{
    FILE *fp;
    int32_t i,n,cmpval,appendflag = 0;
    void *refdata,*itemdata;
    cJSON *json,*newjson;
    char fname[1024];
    (*setfname)(fname,NXTaddr);
    if ( (json= update_MGW_file(&fp,&newjson,fname,jsonstr)) != 0 && newjson != 0 && fp != 0 )
    {
        refdata = (*extract_jsondata)(newjson,arg,arg2);
        if ( refdata != 0 && is_cJSON_Array(json) != 0 && (n= cJSON_GetArraySize(json)) > 0 )
        {
            for (i=0; i<n; i++)
            {
                if ( (itemdata = (*extract_jsondata)(cJSON_GetArrayItem(json,i),arg,arg2)) != 0 )
                {
                    cmpval = (*jsoncmp)(refdata,itemdata);
                    if ( itemdata != 0 ) free(itemdata);
                    if ( cmpval == 0 )
                        break;
                }
            }
            if ( i == n )
                newjson = append_MGW_file(fname,fp,json,newjson), appendflag = 1;
        }
        fclose(fp);
        if ( refdata != 0 ) free(refdata);
        if ( newjson != 0 ) free_json(newjson);
        free_json(json);
    }
    return(appendflag);
}

double get_current_rate(char *base,char *rel)
{
    struct coin777 *coin;
    if ( strcmp(rel,"NXT") == 0 )
    {
        if ( (coin= coin777_find(base,0)) != 0 )
        {
            if ( coin->mgw.NXTconvrate != 0. )
                return(coin->mgw.NXTconvrate);
            else if ( coin->mgw.NXTfee_equiv != 0 && coin->mgw.txfee != 0 )
                return(coin->mgw.NXTfee_equiv / coin->mgw.txfee);
        }
    }
    return(1.);
}

uint64_t MGWtransfer_asset(cJSON **transferjsonp,int32_t forceflag,uint64_t nxt64bits,char *depositors_pubkey,struct coin777 *coin,uint64_t value,char *coinaddr,char *txidstr,uint16_t vout,int32_t *buyNXTp,int32_t deadline)
{
    char buf[MAX_JSON_FIELD],nxtassetidstr[64],numstr[64],assetidstr[64],rsacct[64],NXTaddr[64],comment[MAX_JSON_FIELD],*errjsontxt,*str;
    uint64_t depositid = 0,convamount,total = 0;
    int32_t haspubkey,iter,flag,buyNXT = *buyNXTp;
    double rate;
    cJSON *pair,*errjson,*item;
    expand_nxt64bits(NXTaddr,nxt64bits);
    conv_rsacctstr(rsacct,nxt64bits);
    issue_getpubkey(&haspubkey,rsacct);
    if ( haspubkey != 0 && depositors_pubkey[0] == 0 )
    {
        set_NXTpubkey(depositors_pubkey,NXTaddr);
        printf("set pubkey.(%s)\n",depositors_pubkey);
    }
    expand_nxt64bits(NXTaddr,nxt64bits);
    //sprintf(comment,"{\"coin\":\"%s\",\"coinaddr\":\"%s\",\"cointxid\":\"%s\",\"coinv\":%u,\"amount\":\"%.8f\",\"sender\":\"%s\",\"receiver\":\"%llu\",\"timestamp\":%u,\"quantity\":\"%llu\"}",coin->name,coinaddr,txidstr,vout,dstr(value),SUPERNET.NXTADDR,(long long)nxt64bits,(uint32_t)time(NULL),(long long)(value/coin->mgw.ap_mult));
    sprintf(comment,"{\"coin\":\"%s\",\"coinaddr\":\"%s\",\"cointxid\":\"%s\",\"coinv\":%u,\"amount\":\"%.8f\"}",coin->name,coinaddr,txidstr,vout,dstr(value));//,SUPERNET.NXTADDR,(long long)nxt64bits,(uint32_t)time(NULL),(long long)(value/coin->mgw.ap_mult));
    pair = cJSON_Parse(comment);
    //cJSON_AddItemToObject(pair,"NXT",cJSON_CreateString(NXTaddr));
    printf("forceflag.%d haspubkey.%d >>>>>>>>>>>>>> Need to transfer %.8f %ld assetoshis | %s to %llu for (%s) %s\n",forceflag,haspubkey,dstr(value),(long)(value/coin->mgw.ap_mult),coin->name,(long long)nxt64bits,txidstr,comment);
    total += value;
    convamount = 0;
    if ( haspubkey == 0 && buyNXT > 0 )
    {
        if ( (rate = get_current_rate(coin->name,"NXT")) != 0. )
        {
            if ( buyNXT > MAX_BUYNXT )
                buyNXT = MAX_BUYNXT;
            convamount = ((double)(buyNXT+2) * SATOSHIDEN) / rate; // 2 NXT extra to cover the 2 NXT txfees
            if ( convamount >= value )
            {
                convamount = value / 2;
                buyNXT = ((convamount * rate) / SATOSHIDEN);
            }
            cJSON_AddItemToObject(pair,"rate",cJSON_CreateNumber(rate));
            cJSON_AddItemToObject(pair,"conv",cJSON_CreateNumber(dstr(convamount)));
            cJSON_AddItemToObject(pair,"buyNXT",cJSON_CreateNumber(buyNXT));
            value -= convamount;
        }
    } else buyNXT = 0;
    if ( forceflag > 0 && (value > 0 || convamount > 0) )
    {
        flag = 0;
        expand_nxt64bits(nxtassetidstr,NXT_ASSETID);
        for (iter=(value==0); iter<2; iter++)
        {
            errjsontxt = 0;
            str = cJSON_Print(pair);
            _stripwhite(str,' ');
            expand_nxt64bits(assetidstr,coin->mgw.assetidbits);
            depositid = issue_transferAsset(&errjsontxt,0,SUPERNET.NXTACCTSECRET,NXTaddr,(iter == 0) ? assetidstr : nxtassetidstr,(iter == 0) ? (value/coin->mgw.ap_mult) : buyNXT*SATOSHIDEN,MIN_NQTFEE,deadline,str,depositors_pubkey);
            free(str);
            if ( depositid != 0 && errjsontxt == 0 )
            {
                printf("%s worked.%llu\n",(iter == 0) ? "deposit" : "convert",(long long)depositid);
                if ( iter == 1 )
                    *buyNXTp = buyNXT = 0;
                flag++;
                //add_pendingxfer(0,depositid);
                if ( transferjsonp != 0 )
                {
                    if ( *transferjsonp == 0 )
                        *transferjsonp = cJSON_CreateArray();
                    sprintf(numstr,"%llu",(long long)depositid);
                    cJSON_AddItemToObject(pair,(iter == 0) ? "depositid" : "convertid",cJSON_CreateString(numstr));
                }
            }
            else if ( errjsontxt != 0 )
            {
                printf("%s failed.(%s)\n",(iter == 0) ? "deposit" : "convert",errjsontxt);
                if ( 1 && (errjson= cJSON_Parse(errjsontxt)) != 0 )
                {
                    if ( (item= cJSON_GetObjectItem(errjson,"error")) != 0 )
                    {
                        copy_cJSON(buf,item);
                        cJSON_AddItemToObject(pair,(iter == 0) ? "depositerror" : "converterror",cJSON_CreateString(buf));
                    }
                    free_json(errjson);
                }
                else cJSON_AddItemToObject(pair,(iter == 0) ? "depositerror" : "converterror",cJSON_CreateString(errjsontxt));
                free(errjsontxt);
            }
            if ( buyNXT == 0 )
                break;
        }
        if ( flag != 0 )
        {
            str = cJSON_Print(pair);
            _stripwhite(str,' ');
            fprintf(stderr,"updatedeposit.ALL (%s)\n",str);
            update_MGW_jsonfile(set_MGW_depositfname,extract_jsonints,jsonstrcmp,0,str,"coinv","cointxind");
            fprintf(stderr,"updatedeposit.%s (%s)\n",NXTaddr,str);
            update_MGW_jsonfile(set_MGW_depositfname,extract_jsonints,jsonstrcmp,NXTaddr,str,"coinv","cointxind");
            free(str);
        }
    }
    if ( transferjsonp != 0 )
        cJSON_AddItemToArray(*transferjsonp,pair);
    else free_json(pair);
    return(depositid);
}

// transfer approved, transfer pending, transfer completed, added to virtual balance, selected, spent
// set of unspents - deposit completed - pending transfer -> start transfer
// deposit completed -> pool for withdraws

int32_t _valid_txamount(struct mgw777 *mgw,uint64_t value,char *coinaddr)
{
    if ( value >= MIN_DEPOSIT_FACTOR * (mgw->txfee + mgw->NXTfee_equiv) )
    {
        if ( coinaddr == 0 || (strcmp(coinaddr,mgw->marker) != 0 && strcmp(coinaddr,mgw->marker2) != 0) )
            return(1);
    }
    return(0);
}

int32_t _is_limbo_redeem(struct mgw777 *mgw,uint64_t redeemtxidbits)
{
    char str[64];
    expand_nxt64bits(str,redeemtxidbits);
    return(in_jsonarray(mgw->limbo,str));
}
    
int32_t mgw_depositstatus(struct coin777 *coin,struct multisig_addr *msig,char *txidstr,int32_t vout)
{
    int32_t i,n,flag = 0; struct extra_info extra;
    NXT_revassettxid(&extra,coin->mgw.assetidbits,0), n = extra.ind;
    for (i=1; i<=n; i++)
    {
        if ( NXT_revassettxid(&extra,coin->mgw.assetidbits,i) == sizeof(extra) )
        {
            //printf("(%d) ",flag);
            if ( (extra.flags & MGW_DEPOSITDONE) != 0 )
            {
                if ( extra.vout == vout && strcmp(txidstr,extra.coindata) == 0 )
                {
                    //  printf("pendingxfer.(%s).v%d vs (%s).v%d\n",extra.coindata,extra.vout,txidstr,vout);
                    flag = MGW_DEPOSITDONE;
                    break;
                }
            }
            else if ( (extra.flags & MGW_IGNORE) != 0 )
                flag = MGW_IGNORE;
        } else printf("error loading assettxid[%d] for %llu\n",i,(long long)coin->mgw.assetidbits);
    }
    //printf("n.%d ",n);
    return(flag);
}

int32_t mgw_isinternal(struct coin777 *coin,struct multisig_addr *msig,uint32_t addrind,uint32_t unspentind,char *txidstr,int32_t vout)
{
    char txidstr0[1024]; int32_t vout0; uint32_t txidind0,addrind0; struct unspent_info U;
    if ( vout == 0 )
        addrind0 = addrind;
    else if ( (vout0= coin777_unspentmap(&txidind0,txidstr0,coin,unspentind - vout)) == 0 )
    {
        if ( strcmp(txidstr,txidstr0) == 0 )
        {
            if ( coin777_RWmmap(0,&U,coin,&coin->ramchain.unspents,unspentind - vout) == 0 )
            {
                if ( U.addrind == coin->mgw.marker_addrind || U.addrind == coin->mgw.marker2_addrind )
                {
                    printf("U.%u -> (%s).v%u is mgw_isinternal\n",unspentind,txidstr,vout);
                    return(MGW_ISINTERNAL);
                }
                else return(0);
            }
            printf("mgw_isinternal error loading unspent_info %u - v%d\n",unspentind,vout);
            return(0);
        }
        printf("mgw_isinternal mismatched txidstr.(%s) vs (%s)\n",txidstr,txidstr0);
        return(0);
    }
    else printf("got vout.%d instead of expected 0 from unspentind.%d - %d\n",vout0,unspentind,vout);
    return(0);
}

uint64_t mgw_is_mgwtx(struct coin777 *coin,uint32_t txidind)
{
    struct unspent_info U; struct coin777_addrinfo A; struct spend_info S; bits256 txid; struct multisig_addr *msig;
    uint8_t script[4096],*scriptptr; char scriptstr[8192],txidstr[128],buf[8192],zero12[12];
    uint32_t txoffsets[2],nexttxoffsets[2],unspentind,spendind; uint64_t redeemtxid = 0; int32_t j,scriptlen,vout,len;
    if ( coin777_RWmmap(0,txoffsets,coin,&coin->ramchain.txoffsets,txidind) == 0 && coin777_RWmmap(0,nexttxoffsets,coin,&coin->ramchain.txoffsets,txidind+1) == 0 )
    {
        coin777_RWmmap(0,&txid,coin,&coin->ramchain.txidbits,txidind);
        init_hexbytes_noT(txidstr,txid.bytes,sizeof(txid));
        for (spendind=txoffsets[1]; spendind<nexttxoffsets[1]; spendind++)
        {
            if ( coin777_RWmmap(0,&S,coin,&coin->ramchain.spends,spendind) == 0 )
            {
                if ( coin777_RWmmap(0,&U,coin,&coin->ramchain.unspents,S.unspentind) == 0 && coin777_RWmmap(0,&A,coin,&coin->ramchain.addrinfos,U.addrind) == 0 )
                {
                    if ( (msig= find_msigaddr((struct multisig_addr *)buf,&len,coin->name,A.coinaddr)) == 0 )
                        return(0);
                } else printf("couldnt find spend ind.%u\n",S.unspentind);
            } else printf("error getting spendind.%u\n",spendind);
        }
        printf("MGW tx (%s) numvouts.%d: ",txidstr,nexttxoffsets[0] - txoffsets[0]);
        redeemtxid = 1;
        memset(zero12,0,sizeof(zero12));
        for (unspentind=txoffsets[0],vout=0; unspentind<nexttxoffsets[0]; unspentind++,vout++)
        {
            if ( coin777_RWmmap(0,&U,coin,&coin->ramchain.unspents,unspentind) == 0 && coin777_RWmmap(0,&A,coin,&coin->ramchain.addrinfos,U.addrind) == 0 )
            {
                if ( (scriptptr= coin777_scriptptr(&A)) != 0 )
                    init_hexbytes_noT(scriptstr,scriptptr,A.scriptlen);
                else coin777_scriptstr(coin,scriptstr,sizeof(scriptstr),U.rawind_or_blocknum,U.addrind);
                scriptlen = ((int32_t)strlen(scriptstr) >> 1);
                decode_hex(script,scriptlen,scriptstr);
                if ( script[0] == 0x76 && script[1] == 0xa9 && script[2] == 0x14 && memcmp(&script[11],zero12,12) == 0 )
                {
                    scriptptr = &script[3];
                    for (redeemtxid=j=0; j<(int32_t)sizeof(uint64_t); j++)
                        redeemtxid <<= 8, redeemtxid |= (*scriptptr++ & 0xff);
                    printf("(v%d %.8f REDEEMTXID.%llu) ",vout,dstr(U.value),(long long)redeemtxid);
                }
                printf("[a%d %.8f] ",U.addrind,dstr(U.value));
            } else printf("couldnt find unspentind.%u\n",unspentind);
        }
    } else printf("cant find txoffsets[txidind.%u]\n",txidind);
    return(redeemtxid);
}

int32_t mgw_update_redeem(struct mgw777 *mgw,struct extra_info *extra)
{
    uint32_t txidind,addrind = 0,firstblocknum; int32_t i,vout; uint64_t redeemtxid; char txidstr[256];
    struct coin777_Lentry L; struct addrtx_info ATX; struct coin777 *coin = coin777_find(mgw->coinstr,0);
    if ( coin != 0 && coin->ramchain.readyflag != 0 && (extra->flags & MGW_PENDINGREDEEM) != 0 )
    {
        if ( (addrind= coin777_addrind(&firstblocknum,coin,extra->coindata)) != 0 && coin777_RWmmap(0,&L,coin,&coin->ramchain.ledger,addrind) == 0 )
        {
            for (i=0; i<L.numaddrtx; i++)
            {
                coin777_RWaddrtx(0,coin,addrind,&ATX,&L,i);
                if ( (vout= coin777_unspentmap(&txidind,txidstr,coin,ATX.unspentind)) >= 0 )
                {
                    if ( (redeemtxid= mgw_is_mgwtx(coin,txidind)) == extra->txidbits )
                    {
                        printf("MATCHED REDEEM!\n");
                        return(MGW_WITHDRAWDONE);
                    }
                    break;
                } else printf("(%s.v%d != %s)\n",txidstr,vout,extra->coindata);
            }
            printf("PENDING WITHDRAW: (%llu %.8f -> %s) addrind.%u numaddrtx.%d\n",(long long)extra->txidbits,dstr(extra->amount),extra->coindata,addrind,L.numaddrtx);
        } else printf("skip flag.%d (%s).v%d\n",extra->flags,extra->coindata,extra->vout);
    } else printf("cant find MGW_PENDINGREDEEM (%s) (%llu %.8f)\n",extra->coindata,(long long)extra->txidbits,dstr(extra->amount));
    return(0);
}

int32_t mgw_unspentkey(uint8_t *key,int32_t maxlen,char *txidstr,uint16_t vout)
{
    int32_t slen;
    slen = (int32_t)strlen(txidstr) >> 1;
    memcpy(key,&vout,sizeof(vout)), decode_hex(&key[sizeof(vout)],slen,txidstr), slen += sizeof(vout);
    return(slen);
}

int32_t mgw_unspentstatus(char *txidstr,uint16_t vout)
{
    uint8_t key[1024]; int32_t status,keylen,len = sizeof(status);
    keylen = mgw_unspentkey(key,sizeof(key),txidstr,vout);
    if ( db777_read(&status,&len,0,DB_MGW,key,keylen,0) != 0 )
        return(status);
    return(0);
}

int32_t mgw_markunspent(char *txidstr,int32_t vout,int32_t status)
{
    uint8_t key[1024]; int32_t keylen;
    if ( status < 0 )
        status = MGW_ERRORSTATUS;
    status |= mgw_unspentstatus(txidstr,vout);
    keylen = mgw_unspentkey(key,sizeof(key),txidstr,vout);
    printf("(%s v%d) <- MGW status.%d\n",txidstr,vout,status);
    return(db777_write(0,DB_MGW,key,keylen,&status,sizeof(status)));
}

uint64_t mgw_unspentsfunc(struct coin777 *coin,void *args,uint32_t addrind,struct addrtx_info *unspents,int32_t num,uint64_t balance)
{
    struct multisig_addr *msig = args;
    int32_t i,Ustatus,status,vout; uint32_t unspentind,txidind; char txidstr[512]; uint64_t nxt64bits,atx_value,sum = 0; struct unspent_info U;
    for (i=0; i<num; i++)
    {
        unspentind = unspents[i].unspentind, unspents[i].spendind = 1;
        atx_value = coin777_Uvalue(&U,coin,unspentind);
        if ( (vout= coin777_unspentmap(&txidind,txidstr,coin,unspentind)) >= 0 )
        {
            Ustatus = mgw_unspentstatus(txidstr,vout);
            if ( (Ustatus & (MGW_DEPOSITDONE | MGW_ISINTERNAL | MGW_IGNORE)) == 0 )
            {
                if ( (status= mgw_isinternal(coin,msig,addrind,unspentind,txidstr,vout)) != 0 )
                {
                    printf("ISINTERNAL.%u (%s).v%d %.8f -> %s | Ustatus.%d status.%d\n",unspentind,txidstr,vout,dstr(atx_value),msig->multisigaddr,Ustatus,status);
                    mgw_markunspent(txidstr,vout,Ustatus | MGW_ISINTERNAL);
                }
                else
                {
                    if ( (status= mgw_depositstatus(coin,msig,txidstr,vout)) != 0 )
                    {
                        if ( (status & MGW_DEPOSITDONE) != 0 )
                        {
                            printf("DEPOSIT DONE.%u (%s).v%d %.8f -> %s Ustatus.%d status.%d\n",unspentind,txidstr,vout,dstr(atx_value),msig->multisigaddr,Ustatus,status);
                            mgw_markunspent(txidstr,vout,Ustatus | status);
                        }
                        else if ( (status & MGW_IGNORE) != 0 )
                        {
                            printf("MGW_IGNORE.%u (%s).v%d %.8f -> %s Ustatus.%d status.%d\n",unspentind,txidstr,vout,dstr(atx_value),msig->multisigaddr,Ustatus,status);
                            mgw_markunspent(txidstr,vout,Ustatus | status);
                        }
                        else printf("UNKNOWN.%u (%s).v%d %.8f -> %s Ustatus.%d status.%d\n",unspentind,txidstr,vout,dstr(atx_value),msig->multisigaddr,Ustatus,status);
                    }
                    else
                    {
                        // withdraw 11364111978695678059
                        printf("unhandled case.%u (%s).v%d %.8f -> %s | Ustatus.%d status.%d\n",unspentind,txidstr,vout,dstr(atx_value),msig->multisigaddr,Ustatus,status);
                    }
                }
            }
            else
            {
                nxt64bits = calc_nxt64bits(msig->NXTaddr);
                if ( (Ustatus & MGW_ISINTERNAL) != 0 )
                {
                    sum += U.value;
                    printf("ISINTERNAL.%u (%s).v%d %.8f -> %s\n",unspentind,txidstr,vout,dstr(atx_value),msig->multisigaddr);
                }
                else if ( (Ustatus & MGW_DEPOSITDONE) == 0 )
                {
                    if ( (Ustatus & MGW_PENDINGXFER) != 0 )
                    {
                        printf("G%d PENDINGXFER.%u (%s).v%d %.8f -> %s\n",(int32_t)(nxt64bits % msig->n),unspentind,txidstr,vout,dstr(atx_value),msig->multisigaddr);
                        if ( (status= mgw_depositstatus(coin,msig,txidstr,vout)) == MGW_DEPOSITDONE )
                            mgw_markunspent(txidstr,vout,Ustatus | MGW_DEPOSITDONE);
                    }
                    else if ( coin->mgw.firstunspentind == 0 || unspentind >= coin->mgw.firstunspentind )
                    {
                        printf("pending deposit.%u (%s).v%d %.8f -> %s | Ustatus.%d status.%d\n",unspentind,txidstr,vout,dstr(atx_value),msig->multisigaddr,Ustatus,status);
                        if ( (nxt64bits % msig->n) == SUPERNET.gatewayid )
                        {
                            if ( MGWtransfer_asset(0,1,nxt64bits,msig->NXTpubkey,coin,atx_value,msig->multisigaddr,txidstr,vout,&msig->buyNXT,DEPOSIT_XFER_DURATION) != 0 )
                                mgw_markunspent(txidstr,vout,Ustatus | MGW_PENDINGXFER);
                        } else mgw_markunspent(txidstr,vout,Ustatus | MGW_PENDINGXFER);
                    }
                    else
                    {
                        sum += U.value;
                        mgw_markunspent(txidstr,vout,Ustatus | MGW_DEPOSITDONE);
                    }
                }
                else if ( (Ustatus & MGW_DEPOSITDONE) != 0 )
                    sum += U.value;
            }
        } else printf("error getting unspendind.%u\n",unspentind);
    }
    return(sum);
}

uint64_t mgw_calc_unspent(char *smallestaddr,char *smallestaddrB,struct coin777 *coin)
{
    struct multisig_addr **msigs; int32_t i,n = 0,m=0; uint32_t firstblocknum; uint64_t circulation,smallest,val,unspent = 0;
    ramchain_prepare(coin,&coin->ramchain);
    coin->ramchain.paused = 0;
    smallestaddr[0] = smallestaddrB[0] = 0;
    if ( coin == 0 )
    {
        printf("mgw_calc_MGWunspent: no coin777\n");
        return(0);
    }
    if ( coin->mgw.marker_addrind == 0 && coin->mgw.marker != 0 )
        coin->mgw.marker_addrind = coin777_addrind(&firstblocknum,coin,coin->mgw.marker);
    if ( coin->mgw.marker2_addrind == 0 && coin->mgw.marker2 != 0 )
        coin->mgw.marker2_addrind = coin777_addrind(&firstblocknum,coin,coin->mgw.marker2);
    if ( (msigs= (struct multisig_addr **)db777_copy_all(&n,DB_msigs,"value",0)) != 0 )
    {
        for (smallest=i=m=0; i<n; i++)
        {
            if ( msigs[i]->sig != stringbits("multisig") )
            {
                free(msigs[i]);
                continue;
            }
            if ( strcmp(msigs[i]->coinstr,coin->name) == 0 && (val= coin777_unspents(mgw_unspentsfunc,coin,msigs[i]->multisigaddr,msigs[i])) != 0 )
            {
                m++;
                unspent += val;
                if ( smallest == 0 || val < smallest )
                {
                    smallest = val;
                    strcpy(smallestaddrB,smallestaddr);
                    strcpy(smallestaddr,msigs[i]->multisigaddr);
                }
                else if ( smallestaddrB[0] == 0 && strcmp(smallestaddr,msigs[i]->multisigaddr) != 0 )
                    strcpy(smallestaddrB,msigs[i]->multisigaddr);
            }
            free(msigs[i]);
        }
        free(msigs);
        if ( Debuglevel > 2 )
            printf("smallest (%s %.8f)\n",smallestaddr,dstr(smallest));
    }
    circulation = calc_circulation(0,&coin->mgw,0);
    printf("%s circulation %.8f vs unspents %.8f [%.8f] nummsigs.%d\n",coin->name,dstr(circulation),dstr(unspent),dstr(unspent) - dstr(circulation),m);
    return(unspent);
}

int32_t make_MGWbus(uint16_t port,char *bindaddr,char serverips[MAX_MGWSERVERS][64],int32_t n)
{
    char tcpaddr[64];
    int32_t i,err,sock,timeout = 1;
    if ( (sock= nn_socket(AF_SP,NN_BUS)) < 0 )
    {
        printf("error getting socket.%d %s\n",sock,nn_strerror(nn_errno()));
        return(-1);
    }
    if ( bindaddr != 0 && bindaddr[0] != 0 )
    {
        sprintf(tcpaddr,"tcp://%s:%d",bindaddr,port);
        printf("MGW bind.(%s)\n",tcpaddr);
        if ( (err= nn_bind(sock,tcpaddr)) < 0 )
        {
            printf("error binding socket.%d %s\n",sock,nn_strerror(nn_errno()));
            return(-1);
        }
        if ( 0 && (err= nn_connect(sock,tcpaddr)) < 0 )
        {
            printf("error nn_connect (%s <-> %s) socket.%d %s\n",bindaddr,tcpaddr,sock,nn_strerror(nn_errno()));
            return(-1);
        }
        if ( timeout > 0 && nn_setsockopt(sock,NN_SOL_SOCKET,NN_RCVTIMEO,&timeout,sizeof(timeout)) < 0 )
        {
            printf("error nn_setsockopt socket.%d %s\n",sock,nn_strerror(nn_errno()));
            return(-1);
        }
        for (i=0; i<n; i++)
        {
            if ( serverips[i] != 0 && serverips[i][0] != 0 && strcmp(bindaddr,serverips[i]) != 0 )
            {
                sprintf(tcpaddr,"tcp://%s:%d",serverips[i],port);
                printf("conn.(%s) ",tcpaddr);
                if ( (err= nn_connect(sock,tcpaddr)) < 0 )
                {
                    printf("error nn_connect (%s <-> %s) socket.%d %s\n",bindaddr,tcpaddr,sock,nn_strerror(nn_errno()));
                    return(-1);
                }
            }
        }
    } else nn_shutdown(sock,0), sock = -1;
    return(sock);
}

int32_t PLUGNAME(_process_json)(struct plugin_info *plugin,uint64_t tag,char *retbuf,int32_t maxlen,char *jsonstr,cJSON *json,int32_t initflag)
{
    char NXTaddr[64],nxtaddr[64],ipaddr[64],*resultstr,*coinstr,*methodstr,*retstr = 0; int32_t i,j,n; cJSON *array; uint64_t nxt64bits;
    retbuf[0] = 0;
    printf("<<<<<<<<<<<< INSIDE PLUGIN! process %s\n",plugin->name);
    if ( initflag > 0 )
    {
        strcpy(retbuf,"{\"result\":\"return JSON init\"}");
        MGW.issuers[MGW.numissuers++] = calc_nxt64bits("423766016895692955");//conv_rsacctstr("NXT-JXRD-GKMR-WD9Y-83CK7",0);
        MGW.issuers[MGW.numissuers++] = calc_nxt64bits("12240549928875772593");//conv_rsacctstr("NXT-3TKA-UH62-478B-DQU6K",0);
        MGW.issuers[MGW.numissuers++] = calc_nxt64bits("8279528579993996036");//conv_rsacctstr("NXT-5294-T9F6-WAWK-9V7WM",0);
        if ( (array= cJSON_GetObjectItem(json,"issuers")) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
        {
            for (i=0; i<n; i++)
            {
                copy_cJSON(NXTaddr,cJSON_GetArrayItem(array,i));
                nxt64bits = calc_nxt64bits(NXTaddr);//conv_rsacctstr(NXTaddr,0);
                for (j=0; j<MGW.numissuers; j++)
                    if ( nxt64bits == MGW.issuers[j] )
                        break;
                if ( j == MGW.numissuers )
                    MGW.issuers[MGW.numissuers++] = nxt64bits;
            }
        }
        MGW.port = get_API_int(cJSON_GetObjectItem(json,"MGWport"),7643);
        MGW.M = get_API_int(cJSON_GetObjectItem(json,"M"),2);
        if ( (array= cJSON_GetObjectItem(json,"MGWservers")) != 0 && (n= cJSON_GetArraySize(array)) > 0 && (n & 1) == 0 )
        {
            for (i=j=0; i<n/2&&i<MAX_MGWSERVERS; i++)
            {
                copy_cJSON(ipaddr,cJSON_GetArrayItem(array,i<<1));
                copy_cJSON(nxtaddr,cJSON_GetArrayItem(array,(i<<1)+1));
                if ( nxtaddr[0] != 0 )
                    nxt64bits = conv_acctstr(nxtaddr);
                else nxt64bits = 0;
                if ( strcmp(ipaddr,MGW.bridgeipaddr) != 0 )
                {
                    MGW.srv64bits[j] = nxt64bits;
                    strcpy(MGW.serverips[j],ipaddr);
                    printf("%d.(%s).%llu ",j,ipaddr,(long long)MGW.srv64bits[j]);
                    j++;
                }
            }
            printf("MGWipaddrs: %s %s %s\n",MGW.serverips[0],MGW.serverips[1],MGW.serverips[2]);
            if ( SUPERNET.gatewayid >= 0 && SUPERNET.numgateways )
            {
                strcpy(SUPERNET.myipaddr,MGW.serverips[SUPERNET.gatewayid]);
            }
            //printf("j.%d M.%d N.%d n.%d (%s).%s gateway.%d\n",j,COINS.M,COINS.N,n,COINS.myipaddr,COINS.myNXTaddr,COINS.gatewayid);
            if ( j != SUPERNET.numgateways )
                sprintf(retbuf+1,"{\"warning\":\"mismatched servers\",\"details\":\"n.%d j.%d vs M.%d N.%d\",",n,j,MGW.M,SUPERNET.numgateways);
            else if ( SUPERNET.gatewayid >= 0 )
            {
                //strcpy(MGW.serverips[SUPERNET.numgateways],MGW.bridgeipaddr);
                //MGW.srv64bits[SUPERNET.numgateways] = calc_nxt64bits(MGW.bridgeacct);
                //MGW.all.socks.both.bus = make_MGWbus(MGW.port,SUPERNET.myipaddr,MGW.serverips,SUPERNET.numgateways+1*0);
            }
        }
        MGW.readyflag = 1;
        plugin->allowremote = 1;
    }
    else
    {
        if ( plugin_result(retbuf,json,tag) > 0 )
            return((int32_t)strlen(retbuf));
        resultstr = cJSON_str(cJSON_GetObjectItem(json,"result"));
        methodstr = cJSON_str(cJSON_GetObjectItem(json,"method"));
        coinstr = cJSON_str(cJSON_GetObjectItem(json,"coin"));
        if ( methodstr == 0 || methodstr[0] == 0 || SUPERNET.gatewayid < 0 )
        {
            printf("(%s) has not method or not a gateway node %d\n",jsonstr,SUPERNET.gatewayid);
            return(0);
        }
        printf("MGW.(%s) for (%s)\n",methodstr,coinstr!=0?coinstr:"");
        if ( resultstr != 0 && strcmp(resultstr,"registered") == 0 )
        {
            plugin->registered = 1;
            strcpy(retbuf,"{\"result\":\"activated\"}");
        }
        else if ( strcmp(methodstr,"msigaddr") == 0 )
        {
            if ( SUPERNET.gatewayid >= 0 )
                retstr = devMGW_command(jsonstr,json);
        }
        else if ( strcmp(methodstr,"myacctpubkeys") == 0 )
            process_acctpubkeys(retbuf,jsonstr,json);
        if ( retstr != 0 )
        {
            strcpy(retbuf,retstr);
            free(retstr);
        }
    }
    return((int32_t)strlen(retbuf));
}

int32_t PLUGNAME(_shutdown)(struct plugin_info *plugin,int32_t retcode)
{
    if ( retcode == 0 )  // this means parent process died, otherwise _process_json returned negative value
    {
    }
    return(retcode);
}
#include "../plugin777.c"

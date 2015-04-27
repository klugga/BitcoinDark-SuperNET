//
//  msig.c
//  crypto777
//
//  Created by James on 4/9/15.
//  Copyright (c) 2015 jl777. All rights reserved.
//

#ifdef DEFINES_ONLY
#ifndef crypto777_msig_h
#define crypto777_msig_h
#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "uthash.h"
#include "cJSON.h"
#include "db777.c"
#include "NXT777.c"
#include "bitcoind.c"
#include "system777.c"
#include "storage.c"
#include "cointx.c"
#include "gen1auth.c"

//struct storage_header { uint32_t size,createtime; uint64_t keyhash; };
struct pubkey_info { uint64_t nxt64bits; uint32_t ipbits; char pubkey[256],coinaddr[128]; };
struct multisig_addr
{
    //struct storage_header H;
    UT_hash_handle hh;
    char NXTaddr[MAX_NXTADDR_LEN],multisigaddr[MAX_COINADDR_LEN],NXTpubkey[96],redeemScript[2048],coinstr[16],email[128];
    uint64_t sender,modified;
    int32_t size,m,n,created,valid,buyNXT;
    struct pubkey_info pubkeys[];
};

char *createmultisig_json_params(struct pubkey_info *pubkeys,int32_t m,int32_t n,char *acctparm);
int32_t update_MGW_jsonfile(void (*setfname)(char *fname,char *NXTaddr),void *(*extract_jsondata)(cJSON *item,void *arg,void *arg2),int32_t (*jsoncmp)(void *ref,void *item),char *NXTaddr,char *jsonstr,void *arg,void *arg2);
void set_MGW_depositfname(char *fname,char *NXTaddr);
int32_t _map_msigaddr(char *redeemScript,char *coinstr,char *serverport,char *userpass,char *normaladdr,char *msigaddr,int32_t gatewayid,int32_t numgateways); //could map to rawind, but this is rarely called
struct multisig_addr *finalize_msig(struct multisig_addr *msig,uint64_t *srvbits,uint64_t refbits);

char *genmultisig(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *coinstr,char *refacct,int32_t M,int32_t N,uint64_t *srv64bits,int32_t n,char *userpubkey,char *email,uint32_t buyNXT);
char *setmultisig(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,char *origargstr);
char *getmsigpubkey(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,char *coinstr,char *refNXTaddr,char *myacctcoinaddr,char *mypubkey);
char *setmsigpubkey(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,char *coinstr,char *refNXTaddr,char *acctcoinaddr,char *userpubkey);
struct multisig_addr *find_msigaddr(int32_t *lenp,char *coinstr,char *NXTaddr,char *msigaddr);
int32_t save_msigaddr(char *coinstr,char *NXTaddr,struct multisig_addr *msig,int32_t len);

char *getmsigpubkey(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,char *coinstr,char *refNXTaddr,char *myacctcoinaddr,char *mypubkey);
char *setmsigpubkey(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,char *coinstr,char *refNXTaddr,char *acctcoinaddr,char *userpubkey);
char *setmultisig(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,char *origargstr);
char *genmultisig(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *coinstr,char *refacct,int32_t M,int32_t N,uint64_t *srv64bits,int32_t n,char *userpubkey,char *email,uint32_t buyNXT);

extern int32_t Debuglevel,Gatewayid,MGW_initdone,Numgateways;

#endif
#else
#ifndef crypto777_msig_c
#define crypto777_msig_c

#ifndef crypto777_msig_h
#define DEFINES_ONLY
#include "msig.c"
#undef DEFINES_ONLY
#endif

#define BTC_COINID 1
#define LTC_COINID 2
#define DOGE_COINID 4
#define BTCD_COINID 8

void multisig_keystr(char *keystr,char *coinstr,char *NXTaddr,char *msigaddr)
{
    if ( msigaddr == 0 || msigaddr[0] == 0 )
        sprintf(keystr,"%s.%s",coinstr,NXTaddr);
    else sprintf(keystr,"%s.%s",coinstr,msigaddr);
}

struct multisig_addr *find_msigaddr(int32_t *lenp,char *coinstr,char *NXTaddr,char *msigaddr)
{
    char keystr[1024];
    multisig_keystr(keystr,coinstr,NXTaddr,msigaddr);
    printf("search_msig.(%s)\n",keystr);
    return(db777_findM(lenp,DB_msigs,keystr));
}

int32_t save_msigaddr(char *coinstr,char *NXTaddr,struct multisig_addr *msig,int32_t len)
{
    char keystr[1024];
    multisig_keystr(keystr,coinstr,NXTaddr,msig->multisigaddr);
    printf("save_msig.(%s)\n",keystr);
    return(db777_add(DB_msigs,keystr,msig,len));
}

int32_t _map_msigaddr(char *redeemScript,char *coinstr,char *serverport,char *userpass,char *normaladdr,char *msigaddr,int32_t gatewayid,int32_t numgateways) //could map to rawind, but this is rarely called
{
    int32_t ismine,len;
    struct multisig_addr *msig;
    redeemScript[0] = normaladdr[0] = 0;
    if ( (msig= find_msigaddr(&len,coinstr,0,msigaddr)) == 0 )
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

struct multisig_addr *alloc_multisig_addr(char *coinstr,int32_t m,int32_t n,char *NXTaddr,char *userpubkey,char *sender)
{
    struct multisig_addr *msig;
    int32_t size = (int32_t)(sizeof(*msig) + n*sizeof(struct pubkey_info));
    msig = calloc(1,size);
    msig->size = size;
    msig->n = n;
    msig->created = (uint32_t)time(NULL);
    if ( sender != 0 && sender[0] != 0 )
        msig->sender = calc_nxt64bits(sender);
    safecopy(msig->coinstr,coinstr,sizeof(msig->coinstr));
    safecopy(msig->NXTaddr,NXTaddr,sizeof(msig->NXTaddr));
    safecopy(msig->NXTpubkey,userpubkey,sizeof(msig->NXTpubkey));
    msig->m = m;
    return(msig);
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

void *extract_jsonmsig(cJSON *item,void *arg,void *arg2)
{
    char sender[MAX_JSON_FIELD];
    copy_cJSON(sender,cJSON_GetObjectItem(item,"sender"));
    return(decode_msigjson(0,item,sender));
}

int32_t jsonmsigcmp(void *ref,void *item) { return(msigcmp(ref,item)); }
int32_t jsonstrcmp(void *ref,void *item) { return(strcmp(ref,item)); }

cJSON *http_search(char *destip,char *type,char *file)
{
    cJSON *json = 0;
    char url[1024],*retstr;
    sprintf(url,"http://%s/%s/%s",destip,type,file);
    if ( (retstr= issue_curl(0,url)) != 0 )
    {
        json = cJSON_Parse(retstr);
        free(retstr);
    }
    return(json);
}

struct multisig_addr *http_search_msig(char *external_NXTaddr,char *external_ipaddr,char *NXTaddr)
{
    int32_t i,n,len;
    cJSON *array;
    struct multisig_addr *msig = 0;
    if ( (array= http_search(external_ipaddr,"MGW/msig",NXTaddr)) != 0 )
    {
        if ( is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
        {
            for (i=0; i<n; i++)
                if ( (msig= decode_msigjson(0,cJSON_GetArrayItem(array,i),external_NXTaddr)) != 0 && (msig= find_msigaddr(&len,msig->coinstr,NXTaddr,msig->multisigaddr)) != 0 )
                    break;
        }
        free_json(array);
    }
    return(msig);
}

void set_MGW_fname(char *fname,char *dirname,char *NXTaddr)
{
    if ( NXTaddr == 0 )
        sprintf(fname,"%s/MGW/%s/ALL",SUPERNET.MGWROOT,dirname);
    else sprintf(fname,"%s/MGW/%s/%s",SUPERNET.MGWROOT,dirname,NXTaddr);
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
    char jsontxt[65536],pubkeyjsontxt[65536],rsacct[64];
    if ( msig != 0 )
    {
        if ( (coin= coin777_find(msig->coinstr)) != 0 )
            gatewayid = coin->gatewayid;
        rsacct[0] = 0;
        conv_rsacctstr(rsacct,calc_nxt64bits(msig->NXTaddr));
        pubkeyjsontxt[0] = 0;
        for (i=0; i<msig->n; i++)
            len += calc_pubkey_jsontxt(truncated,pubkeyjsontxt+strlen(pubkeyjsontxt),&msig->pubkeys[i],(i<(msig->n - 1)) ? ", " : "");
        sprintf(jsontxt,"{%s\"sender\":\"%llu\",\"buyNXT\":%u,\"created\":%u,\"M\":%d,\"N\":%d,\"NXTaddr\":\"%s\",\"NXTpubkey\":\"%s\",\"RS\":\"%s\",\"address\":\"%s\",\"redeemScript\":\"%s\",\"coin\":\"%s\",\"gatewayid\":\"%d\",\"pubkey\":[%s]}",truncated==0?"\"requestType\":\"MGWaddr\",":"",(long long)msig->sender,msig->buyNXT,msig->created,msig->m,msig->n,msig->NXTaddr,msig->NXTpubkey,rsacct,msig->multisigaddr,msig->redeemScript,msig->coinstr,gatewayid,pubkeyjsontxt);
        //if ( (MGW_initdone == 0 && Debuglevel > 2) || MGW_initdone != 0 )
        //    printf("(%s) pubkeys len.%ld msigjsonlen.%ld\n",jsontxt,len,strlen(jsontxt));
        return(clonestr(jsontxt));
    }
    else return(0);
}

int32_t update_MGW_msig(struct multisig_addr *msig,char *sender)
{
    char *jsonstr;
    int32_t appendflag = 0;
    if ( msig != 0 )
    {
        jsonstr = create_multisig_jsonstr(msig,0);
        if ( jsonstr != 0 )
        {
            //if ( (MGW_initdone == 0 && Debuglevel > 2) || MGW_initdone != 0 )
            //   printf("add_MGWaddr(%s) from (%s)\n",jsonstr,sender!=0?sender:"");
            //broadcast_bindAM(msig->NXTaddr,msig,origargstr);
            //update_MGW_msigfile(0,msig,jsonstr);
            // update_MGW_msigfile(msig->NXTaddr,msig,jsonstr);
            update_MGW_jsonfile(set_MGW_msigfname,extract_jsonmsig,jsonmsigcmp,0,jsonstr,0,0);
            appendflag = update_MGW_jsonfile(set_MGW_msigfname,extract_jsonmsig,jsonmsigcmp,msig->NXTaddr,jsonstr,0,0);
            free(jsonstr);
        }
    }
    return(appendflag);
}

struct multisig_addr *finalize_msig(struct multisig_addr *msig,uint64_t *srvbits,uint64_t refbits)
{
    int32_t i,n;
    char acctcoinaddr[1024],pubkey[1024];
    for (i=n=0; i<msig->n; i++)
    {
        printf("i.%d n.%d msig->n.%d NXT.(%s) msig.(%s) %p\n",i,n,msig->n,msig->NXTaddr,msig->multisigaddr,msig);
        if ( srvbits[i] != 0 && refbits != 0 )
        {
            acctcoinaddr[0] = pubkey[0] = 0;
            if ( get_NXT_coininfo(srvbits[i],acctcoinaddr,pubkey,refbits,msig->coinstr) != 0 && acctcoinaddr[0] != 0 && pubkey[0] != 0 )
            {
                strcpy(msig->pubkeys[i].coinaddr,acctcoinaddr);
                strcpy(msig->pubkeys[i].pubkey,pubkey);
                msig->pubkeys[i].nxt64bits = srvbits[i];
                n++;
            }
        }
    }
    if ( n != msig->n )
        free(msig), msig = 0;
    return(msig);
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

struct multisig_addr *gen_multisig_addr(char *sender,int32_t M,int32_t N,char *coinstr,char *serverport,char *userpass,int32_t use_addmultisig,char *refNXTaddr,char *userpubkey,uint64_t *srvbits)
{
    uint64_t refbits;//,srvbits[16];
    int32_t flag = 0;
    struct multisig_addr *msig;
    refbits = calc_nxt64bits(refNXTaddr);
    msig = alloc_multisig_addr(coinstr,M,N,refNXTaddr,userpubkey,sender);
    //for (i=0; i<N; i++)
    //    srvbits[i] = contacts[i]->nxt64bits;
    if ( (msig= finalize_msig(msig,srvbits,refbits)) != 0 )
        flag = issue_createmultisig(msig->multisigaddr,msig->redeemScript,coinstr,serverport,userpass,use_addmultisig,msig);
    if ( flag == 0 )
    {
        free(msig);
        return(0);
    }
    return(msig);
}

int32_t update_MGWaddr(cJSON *argjson,char *sender)
{
    int32_t i,retval = 0;
    uint64_t senderbits;
    struct multisig_addr *msig;
    if  ( (msig= decode_msigjson(0,argjson,sender)) != 0 )
    {
        senderbits = calc_nxt64bits(sender);
        for (i=0; i<msig->n; i++)
        {
            if ( msig->pubkeys[i].nxt64bits == senderbits )
            {
                update_msig_info(msig,1,sender);
                update_MGW_msig(msig,sender);
                retval = 1;
                break;
            }
        }
        free(msig);
    }
    return(retval);
}

int32_t add_MGWaddr(char *previpaddr,char *sender,int32_t valid,char *origargstr)
{
    cJSON *origargjson,*argjson;
    if ( valid > 0 && (origargjson= cJSON_Parse(origargstr)) != 0 )
    {
        if ( is_cJSON_Array(origargjson) != 0 )
            argjson = cJSON_GetArrayItem(origargjson,0);
        else argjson = origargjson;
        return(update_MGWaddr(argjson,sender));
    }
    return(0);
}

char *genmultisig(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *coinstr,char *refacct,int32_t M,int32_t N,uint64_t *srv64bits,int32_t n,char *userpubkey,char *email,uint32_t buyNXT)
{
    struct coin777 *coin;
    uint32_t crc,lastcrc = 0;
    struct multisig_addr *msig;
    char refNXTaddr[64],destNXTaddr[64],mypubkey[1024],myacctcoinaddr[1024],pubkey[1024],acctcoinaddr[1024],buf[1024],*retstr = 0;
    uint64_t my64bits,nxt64bits,refbits = 0;
    int32_t i,iter,len,flag,valid = 0;
    if ( (coin= coin777_find(coinstr)) == 0 )
        return(0);
    my64bits = calc_nxt64bits(NXTaddr);
    refbits = conv_acctstr(refacct);
    expand_nxt64bits(refNXTaddr,refbits);
    //if ( (MGW_initdone == 0 && Debuglevel > 2) || MGW_initdone != 0 )
    printf("GENMULTISIG.%d from (%s) for %s refacct.(%s) %llu %s email.(%s) buyNXT.%u userpub.%s\n",N,previpaddr,coinstr,refacct,(long long)refbits,refNXTaddr,email,buyNXT,userpubkey);// getchar();
    if ( refNXTaddr[0] == 0 )
        return(clonestr("\"error\":\"genmultisig couldnt find refcontact\"}"));
    if ( userpubkey[0] == 0 )
        set_NXTpubkey(userpubkey,refNXTaddr);
    flag = 0;
    myacctcoinaddr[0] = mypubkey[0] = 0;
    for (iter=0; iter<2; iter++)
        for (i=0; i<n; i++)
        {
            //fprintf(stderr,"iter.%d i.%d\n",iter,i);
            if ( (nxt64bits= srv64bits[i]) != 0 )
            {
                if ( iter == 0 && my64bits == nxt64bits )
                {
                    myacctcoinaddr[0] = mypubkey[0] = 0;
                    if ( get_acct_coinaddr(myacctcoinaddr,coinstr,coin->serverport,coin->userpass,refNXTaddr) != 0 && get_pubkey(mypubkey,coinstr,coin->serverport,coin->userpass,myacctcoinaddr) != 0 && myacctcoinaddr[0] != 0 && mypubkey[0] != 0 )
                    {
                        flag++;
                        add_NXT_coininfo(nxt64bits,refbits,coinstr,myacctcoinaddr,mypubkey);
                        valid++;
                    }
                    else printf("error getting msigaddr for (%s) ref.(%s) addr.(%s) pubkey.(%s)\n",coinstr,refNXTaddr,myacctcoinaddr,mypubkey);
                }
                else if ( iter == 1 && my64bits != nxt64bits )
                {
                    acctcoinaddr[0] = pubkey[0] = 0;
                    if ( get_NXT_coininfo(nxt64bits,acctcoinaddr,pubkey,refbits,coinstr) == 0 || acctcoinaddr[0] == 0 || pubkey[0] == 0 )
                    {
                        sprintf(buf,"{\"requestType\":\"plugin\",\"plugin\":\"coins\",\"method\":\"getmsigpubkey\",\"NXT\":\"%s\",\"coin\":\"%s\",\"refNXTaddr\":\"%s\",\"userpubkey\":\"%s\"",NXTaddr,coinstr,refNXTaddr,userpubkey);
                        if ( myacctcoinaddr[0] != 0 && mypubkey[0] != 0 )
                            sprintf(buf+strlen(buf),",\"myaddr\":\"%s\",\"mypubkey\":\"%s\"",myacctcoinaddr,mypubkey);
                        if ( Debuglevel > 2 )
                            printf("SENDREQ.(%s)\n",buf);
                        expand_nxt64bits(destNXTaddr,nxt64bits);
                        if ( (crc= _crc32(0,buf,strlen(buf))) != lastcrc )
                        {
                            sprintf(buf+strlen(buf),",\"tag\":\"%u\")",rand());
                            if ( (len= nn_send(SUPERNET.all.socks.both.bus,buf,(int32_t)strlen(buf)+1,0)) <= 0 )
                                printf("error sending (%s)\n",buf);
                            else printf("sent.(%s).%d\n",buf,len);
                            lastcrc = crc;
                        }
                        /*if ( (str= plugin_method(0,"coins","getmsigpubkey",0,milliseconds(),buf,1,1)) != 0 )
                        {
                            printf("GENMULTISIG sent to MGW bus (%s)\n",buf);
                            free(str);
                        }*/
                    }
                    else
                    {
                        //printf("already have %llu:%llu (%s %s)\n",(long long)contact->nxt64bits,(long long)refbits,acctcoinaddr,pubkey);
                        valid++;
                    }
                }
            }
        }
    if ( (msig= find_NXT_msig(NXTaddr,coinstr,srv64bits,N)) == 0 )
    {
        if ( (msig= gen_multisig_addr(NXTaddr,M,N,coinstr,coin->serverport,coin->userpass,coin->use_addmultisig,refNXTaddr,userpubkey,srv64bits)) != 0 )
        {
            msig->valid = valid;
            safecopy(msig->email,email,sizeof(msig->email));
            msig->buyNXT = buyNXT;
            update_msig_info(msig,1,NXTaddr);
            update_MGW_msig(msig,NXTaddr);
        }
    } else valid = N;
    if ( valid == N && msig != 0 )
    {
        if ( (retstr= create_multisig_jsonstr(msig,0)) != 0 )
        {
            if ( retstr != 0 && previpaddr != 0 && previpaddr[0] != 0 )
            {
                //if ( (MGW_initdone == 0 && Debuglevel > 2) || MGW_initdone > 1 )
                //    printf("retstr.(%s) previp.(%s)\n",retstr,previpaddr);
                //send_to_ipaddr(0,1,previpaddr,retstr,NXTACCTSECRET);
            }
        }
    }
    if ( msig != 0 )
        free(msig);
    if ( valid != N || retstr == 0 )
    {
        sprintf(buf,"{\"error\":\"missing msig info\",\"refacct\":\"%s\",\"coin\":\"%s\",\"M\":%d,\"N\":%d,\"valid\":%d}",refacct,coinstr,M,N,valid);
        retstr = clonestr(buf);
        //printf("%s\n",buf);
    }
    return(retstr);
}

char *getmsigpubkey(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,char *coinstr,char *refNXTaddr,char *myacctcoinaddr,char *mypubkey)
{
    struct coin777 *coin;
    char acctcoinaddr[MAX_JSON_FIELD],pubkey[MAX_JSON_FIELD],buf[MAX_JSON_FIELD];
    if ( coinstr[0] != 0 && refNXTaddr[0] != 0 && (coin= coin777_find(coinstr)) != 0 )
    {
        if ( myacctcoinaddr != 0 && myacctcoinaddr[0] != 0 && mypubkey != 0 && mypubkey[0] != 0 )
            add_NXT_coininfo(calc_nxt64bits(sender),conv_acctstr(refNXTaddr),coinstr,myacctcoinaddr,mypubkey);
        if ( get_acct_coinaddr(myacctcoinaddr,coinstr,coin->serverport,coin->userpass,refNXTaddr) != 0 && get_pubkey(pubkey,coinstr,coin->serverport,coin->userpass,acctcoinaddr) != 0 )
        {
            sprintf(buf,"{\"requestType\":\"setmsigpubkey\",\"NXT\":\"%s\",\"coin\":\"%s\",\"refNXTaddr\":\"%s\",\"addr\":\"%s\",\"userpubkey\":\"%s\",\"tag\":\"%u\"}",NXTaddr,coinstr,refNXTaddr,acctcoinaddr,pubkey,rand());
            if ( nn_send(SUPERNET.all.socks.both.bus,buf,(int32_t)strlen(buf)+1,0) <= 0 )
                printf("error sending (%s)\n",buf);
            /*if ( (str= plugin_method(previpaddr,"coins","setmsigpubkey",0,milliseconds(),buf,1,1)) != 0 )
            {
                printf("GETMSIG sent to MGW bus (%s)\n",buf);
                free(str);
            }*/
        }
    }
    return(clonestr("{\"error\":\"bad getmsigpubkey_func paramater\"}"));
}

char *setmsigpubkey(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,char *coinstr,char *refNXTaddr,char *acctcoinaddr,char *userpubkey)
{
    struct coin777 *coin;
    uint64_t nxt64bits;
    if ( sender[0] != 0 && refNXTaddr[0] != 0 && acctcoinaddr[0] != 0 && userpubkey[0] != 0 && (coin= coin777_find(coinstr)) != 0 )
    {
        if ( (nxt64bits= conv_acctstr(refNXTaddr)) != 0 )
        {
            add_NXT_coininfo(calc_nxt64bits(sender),nxt64bits,coinstr,acctcoinaddr,userpubkey);
            return(clonestr("{\"result\":\"setmsigpubkey added coininfo\"}"));
        }
        return(clonestr("{\"error\":\"setmsigpubkey_func couldnt convert refNXTaddr\"}"));
    }
    return(clonestr("{\"error\":\"bad setmsigpubkey_func paramater\"}"));
}

char *setmultisig(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,char *origargstr)
{
    if ( Debuglevel > 0 )
        printf("MGWaddr_func(%s)\n",origargstr);
    if ( sender[0] != 0 && origargstr[0] != 0 )
        add_MGWaddr(previpaddr,sender,1,origargstr);
    return(clonestr(origargstr));
}

int32_t init_public_msigs()
{
    void *curl_post(void **cHandlep,char *url,char *postfields,char *hdr0,char *hdr1,char *hdr2);
    static void *cHandle;
    char Server_NXTaddr[64],url[1024],*retstr;
    struct multisig_addr *msig;
    cJSON *json;
    int32_t len,i,j,n,added = 0;
    printf("init_public_msigs numgateways.%d gatewayid.%d\n",SUPERNET.numgateways,SUPERNET.gatewayid);
    if ( SUPERNET.gatewayid < 0 || SUPERNET.numgateways <= 0 )
        return(-1);
    for (j=0; j<SUPERNET.numgateways; j++)
    {
        expand_nxt64bits(Server_NXTaddr,SUPERNET.srv64bits[j]);
        sprintf(url,"http://%s/MGW/msig/ALL",SUPERNET.Server_ipaddrs[j]);
        printf("issue.(%s)\n",url);
        if ( (retstr= curl_post(&cHandle,url,0,"",0,0)) != 0 )
        {
            printf("got.(%s)\n",retstr);
            if ( (json= cJSON_Parse(retstr)) != 0 )
            {
                if ( is_cJSON_Array(json) != 0 && (n= cJSON_GetArraySize(json)) > 0 )
                {
                    for (i=0; i<n; i++)
                        if ( (msig= decode_msigjson(0,cJSON_GetArrayItem(json,i),Server_NXTaddr)) != 0 )
                        {
                            if ( find_msigaddr(&len,msig->coinstr,msig->NXTaddr,msig->multisigaddr) == 0 )
                            {
                                printf("ADD.%d %s.(%s) NXT.(%s) NXTpubkey.(%s) (%s)\n",added,msig->coinstr,msig->multisigaddr,msig->NXTaddr,msig->NXTpubkey,msig->pubkeys[0].coinaddr);
                                if ( is_zeroes(msig->NXTpubkey) != 0 )
                                {
                                    set_NXTpubkey(msig->NXTpubkey,msig->NXTaddr);
                                    printf("FIX (%s) NXT.(%s) NXTpubkey.(%s)\n",msig->multisigaddr,msig->NXTaddr,msig->NXTpubkey);
                                }
                                update_msig_info(msig,i == n-1,Server_NXTaddr), added++;
                            }
                        }
                }
                free_json(json);
            }
            free(retstr);
        }
    }
    printf("added.%d multisig addrs\n",added);
    return(added);
}

#endif
#endif
/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2013-2020 Lixin
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <srs_sip_stack.hpp>

#if !defined(SRS_EXPORT_LIBRTMP)

#include <stdio.h>
#include <stdlib.h>
#include <iostream>  
#include <map>

using namespace std;

#include <srs_protocol_io.hpp>
#include <srs_kernel_stream.hpp>
#include <srs_kernel_error.hpp>
#include <srs_kernel_log.hpp>
#include <srs_kernel_consts.hpp>
#include <srs_core_autofree.hpp>
#include <srs_kernel_utility.hpp>
#include <srs_kernel_buffer.hpp>
#include <srs_kernel_codec.hpp>
#include <srs_rtsp_stack.hpp>

unsigned int srs_sip_random(int min,int max)  
{  
    srand(int(time(0)));
    return  rand() % (max - min + 1) + min;
} 

std::string  srs_sip_get_form_to_uri(std::string  msg)
{
    //<sip:34020000002000000001@3402000000>;tag=536961166
    //sip:34020000002000000001@3402000000 

    size_t pos = msg.find("<");
    if (pos == string::npos) {
        return msg;
    }

    msg = msg.substr(pos+1);

    size_t pos2 = msg.find(">");
    if (pos2 == string::npos) {
        return msg;
    }

    msg = msg.substr(0, pos2);
    return msg;
}

std::string srs_sip_get_utc_date()
{
    // clock time
    timeval tv;
    if (gettimeofday(&tv, NULL) == -1) {
        return "";
    }
    
    // to calendar time
    struct tm* tm;
    if ((tm = gmtime(&tv.tv_sec)) == NULL) {
        return "";
    }
    
    //Date: 2020-03-21T14:20:57.638
    std::string utc_date = "";
    char buffer[25] = {0};
    snprintf(buffer, 25,
                "%d-%02d-%02dT%02d:%02d:%02d.%03d",
                1900 + tm->tm_year, 1 + tm->tm_mon, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, (int)(tv.tv_usec / 1000));
    utc_date = buffer;
    return utc_date;
}


std::string srs_sip_get_param(std::string msg, std::string param)
{
    std::vector<std::string>  vec_params = srs_string_split(msg, ";");

    for (vector<string>::iterator it = vec_params.begin(); it != vec_params.end(); ++it) {
        string  value = *it;
        
        size_t pos = value.find(param);
        if (pos == string::npos) {
            continue;
        }

        std::vector<std::string>  v_pram = srs_string_split(value, "=");
        
        if (v_pram.size() > 0) {
            return v_pram.at(1);
        }
    }
    return "";
}

SrsSipRequest::SrsSipRequest()
{
    seq = 0;
    content_length = 0;
    sdp = NULL;
    transport = NULL;

    method = "";
    uri = "";;
    version = "";;
    seq = 0;
    content_type = "";
    content_length = 0;
    call_id = "";
    from = "";
    to = "";
    via = "";
    from_tag = "";
    to_tag = "";
    contact = "";
    user_agent = "";
    branch = "";
    status = "";
    expires = 3600;
    max_forwards = 70;
    www_authenticate = "";
    authorization = "";
    cmdtype = SrsSipCmdRequest;

    host = "127.0.0.1";;
    host_port = 5060;

    serial = "";;
    realm = "";;

    sip_auth_id = "";
    sip_auth_pwd = "";
    sip_username = "";
    peer_ip = "";
    peer_port = 0;
}

SrsSipRequest::~SrsSipRequest()
{
    srs_freep(sdp);
    srs_freep(transport);
}

bool SrsSipRequest::is_register()
{
    return method == SRS_SIP_METHOD_REGISTER;
}

bool SrsSipRequest::is_invite()
{
    return method == SRS_SIP_METHOD_INVITE;
}

bool SrsSipRequest::is_ack()
{
    return method == SRS_SIP_METHOD_ACK;
}

bool SrsSipRequest::is_message()
{
    return method == SRS_SIP_METHOD_MESSAGE;
}

bool SrsSipRequest::is_bye()
{
    return method == SRS_SIP_METHOD_BYE;
}

std::string SrsSipRequest::get_cmdtype_str()
{
    switch(cmdtype) {
        case SrsSipCmdRequest:
            return "request";
        case SrsSipCmdRespone:
            return "respone";
    }

    return "";
}

void SrsSipRequest::copy(SrsSipRequest* src)
{
     if (!src){
         return;
     }
     
     method = src->method;
     uri = src->uri;
     version = src->version;
     seq = src->seq;
     content_type = src->content_type;
     content_length = src->content_length;
     call_id = src->call_id;
     from = src->from;
     to = src->to;
     via = src->via;
     from_tag = src->from_tag;
     to_tag = src->to_tag;
     contact = src->contact;
     user_agent = src->user_agent;
     branch = src->branch;
     status = src->status;
     expires = src->expires;
     max_forwards = src->max_forwards;
     www_authenticate = src->www_authenticate;
     authorization = src->authorization;
     cmdtype = src->cmdtype;

     host = src->host;
     host_port = src->host_port;

     serial = src->serial;
     realm = src->realm;
     
     sip_auth_id = src->sip_auth_id;
     sip_auth_pwd = src->sip_auth_pwd;
     sip_username = src->sip_username;
     peer_ip = src->peer_ip;
     peer_port = src->peer_port;
}

SrsSipStack::SrsSipStack()
{
    buf = new SrsSimpleStream();
}

SrsSipStack::~SrsSipStack()
{
    srs_freep(buf);
}

srs_error_t SrsSipStack::parse_request(SrsSipRequest** preq, const char* recv_msg, int nb_len)
{
    srs_error_t err = srs_success;
    
    SrsSipRequest* req = new SrsSipRequest();
    if ((err = do_parse_request(req, recv_msg)) != srs_success) {
        srs_freep(req);
        return srs_error_wrap(err, "recv message");
    }
    
    *preq = req;
    
    return err;
}

srs_error_t SrsSipStack::do_parse_request(SrsSipRequest* req, const char* recv_msg)
{
    srs_error_t err = srs_success;

    std::vector<std::string> header_body = srs_string_split(recv_msg, SRS_RTSP_CRLFCRLF);
    std::string header = header_body.at(0);
    std::string body = "";

    if (header_body.size() > 1){
       body =  header_body.at(1);
    }

    srs_info("sip: header=%s\n", header.c_str());
    srs_info("sip: body=%s\n", body.c_str());

    // parse one by one.
    char* start = (char*)header.c_str();
    char* end = start + header.size();
    char* p = start;
    char* newline_start = start;
    std::string firstline = "";
    while (p < end) {
        if (p[0] == '\r' && p[1] == '\n'){
            p +=2;
            int linelen = (int)(p - newline_start);
            std::string oneline(newline_start, linelen);
            newline_start = p;

            if (firstline == ""){
                firstline = oneline;
                srs_info("sip: first line=%s", firstline.c_str());
            }else{
                size_t pos = oneline.find(":");
                if (pos != string::npos){
                    if (pos != 0) {
                        //ex: CSeq: 100 MESSAGE  header is 'CSeq:',content is '100 MESSAGE'
                        std::string head = oneline.substr(0, pos+1);
                        std::string content = oneline.substr(pos+1, oneline.length()-pos-1);
                        content = srs_string_replace(content, "\r\n", "");
                        content = srs_string_trim_start(content, " ");
                        char *phead = (char*)head.c_str();
                        
                        if (!strcasecmp(phead, "call-id:")) {
                            std::vector<std::string> vec_callid = srs_string_split(content, " ");
                            req->call_id = vec_callid.at(0);
                        } 
                        else if (!strcasecmp(phead, "contact:")) {
                            req->contact = content;
                        } 
                        else if (!strcasecmp(phead, "content-encoding:")) {
                            srs_trace("sip: message head %s content=%s", phead, content.c_str());
                        } 
                        else if (!strcasecmp(phead, "content-length:")) {
                            req->content_length = strtoul(content.c_str(), NULL, 10);
                        } 
                        else if (!strcasecmp(phead, "content-type:")) {
                            req->content_type = content;
                        } 
                        else if (!strcasecmp(phead, "cseq:")) {
                            std::vector<std::string> vec_seq = srs_string_split(content, " ");
                            req->seq =  strtoul(vec_seq.at(0).c_str(), NULL, 10);
                            req->method = vec_seq.at(1);
                        } 
                        else if (!strcasecmp(phead, "from:")) {
                            content = srs_string_replace(content, "sip:", "");
                            req->from = srs_sip_get_form_to_uri(content.c_str());
                            if (srs_string_contains(content, "tag")) {
                                req->from_tag = srs_sip_get_param(content.c_str(), "tag");
                            }
                        } 
                        else if (!strcasecmp(phead, "to:")) {
                            content = srs_string_replace(content, "sip:", "");
                            req->to = srs_sip_get_form_to_uri(content.c_str());
                            if (srs_string_contains(content, "tag")) {
                                req->to_tag = srs_sip_get_param(content.c_str(), "tag");
                            }
                        } 
                        else if (!strcasecmp(phead, "via:")) {
                            std::vector<std::string> vec_seq = srs_string_split(content, ";");
                            req->via = content;
                            req->branch = srs_sip_get_param(content.c_str(), "branch");
                        } 
                        else if (!strcasecmp(phead, "expires:")){
                            req->expires = strtoul(content.c_str(), NULL, 10);
                        }
                        else if (!strcasecmp(phead, "user-agent:")){
                            req->user_agent = content;
                        } 
                        else if (!strcasecmp(phead, "max-forwards:")){
                            req->max_forwards = strtoul(content.c_str(), NULL, 10);
                        }
                        else if (!strcasecmp(phead, "www-authenticate:")){
                            req->www_authenticate = content;
                        } 
                        else if (!strcasecmp(phead, "authorization:")){
                            req->authorization = content;
                        } 
                        else {
                            //TODO: fixme
                            srs_trace("sip: unkonw message head %s content=%s", phead, content.c_str());
                        }
                   }
                }
            }
        }else{
            p++;
        }
    }
   
    std::vector<std::string>  method_uri_ver = srs_string_split(firstline, " ");
    //respone first line text:SIP/2.0 200 OK
    if (!strcasecmp(method_uri_ver.at(0).c_str(), "sip/2.0")) {
        req->cmdtype = SrsSipCmdRespone;
        //req->method= vec_seq.at(1);
        req->status = method_uri_ver.at(1);
        req->version = method_uri_ver.at(0);
        req->uri = req->from;

        vector<string> str = srs_string_split(req->to, "@");
        req->sip_auth_id = srs_string_replace(str.at(0), "sip:", "");
  
    }else {//request first line text :MESSAGE sip:34020000002000000001@3402000000 SIP/2.0
        req->cmdtype = SrsSipCmdRequest;
        req->method= method_uri_ver.at(0);
        req->uri = method_uri_ver.at(1);
        req->version = method_uri_ver.at(2);

        vector<string> str = srs_string_split(req->from, "@");
        req->sip_auth_id = srs_string_replace(str.at(0), "sip:", "");
    }

    req->sip_username =  req->sip_auth_id;
   
    srs_info("sip: method=%s uri=%s version=%s cmdtype=%s", 
           req->method.c_str(), req->uri.c_str(), req->version.c_str(), req->get_cmdtype_str().c_str());
    srs_info("via=%s", req->via.c_str());
    srs_info("via_branch=%s", req->branch.c_str());
    srs_info("cseq=%d", req->seq);
    srs_info("contact=%s", req->contact.c_str());
    srs_info("from=%s",  req->from.c_str());
    srs_info("to=%s",  req->to.c_str());
    srs_info("callid=%s", req->call_id.c_str());
    srs_info("status=%s", req->status.c_str());
    srs_info("from_tag=%s", req->from_tag.c_str());
    srs_info("to_tag=%s", req->to_tag.c_str());
    srs_info("sip_auth_id=%s", req->sip_auth_id.c_str());

    return err;
}

void SrsSipStack::resp_keepalive(std::stringstream& ss, SrsSipRequest *req){
    ss << SRS_SIP_VERSION <<" 200 OK" << SRS_RTSP_CRLF
    << "Via: " << SRS_SIP_VERSION << "/UDP " << req->host << ":" << req->host_port << ";branch=" << req->branch << SRS_RTSP_CRLF
    << "From: <sip:" << req->from.c_str() << ">;tag=" << req->from_tag << SRS_RTSP_CRLF
    << "To: <sip:"<< req->to.c_str() << ">\r\n"
    << "Call-ID: " << req->call_id << SRS_RTSP_CRLF
    << "CSeq: " << req->seq << " " << req->method << SRS_RTSP_CRLF
    << "Contact: "<< req->contact << SRS_RTSP_CRLF
    << "Max-Forwards: 70" << SRS_RTSP_CRLF
    << "User-Agent: "<< SRS_SIP_USER_AGENT << SRS_RTSP_CRLF
    << "Content-Length: 0" << SRS_RTSP_CRLFCRLF;
}

void SrsSipStack::resp_status(stringstream& ss, SrsSipRequest *req)
{
    if (req->method == "REGISTER"){
        /* 
        //request:  sip-agent-----REGISTER------->sip-server
        REGISTER sip:34020000002000000001@3402000000 SIP/2.0
        Via: SIP/2.0/UDP 192.168.137.11:5060;rport;branch=z9hG4bK1371463273
        From: <sip:34020000001320000003@3402000000>;tag=2043466181
        To: <sip:34020000001320000003@3402000000>
        Call-ID: 1011047669
        CSeq: 1 REGISTER
        Contact: <sip:34020000001320000003@192.168.137.11:5060>
        Max-Forwards: 70
        User-Agent: IP Camera
        Expires: 3600
        Content-Length: 0
        
        //response:  sip-agent<-----200 OK--------sip-server
        SIP/2.0 200 OK
        Via: SIP/2.0/UDP 192.168.137.11:5060;rport;branch=z9hG4bK1371463273
        From: <sip:34020000001320000003@3402000000>
        To: <sip:34020000001320000003@3402000000>
        CSeq: 1 REGISTER
        Call-ID: 1011047669
        Contact: <sip:34020000001320000003@192.168.137.11:5060>
        User-Agent: SRS/4.0.4(Leo)
        Expires: 3600
        Content-Length: 0

        */
        if (req->authorization.empty()){
            //TODO: fixme supoort 401
            //return req_401_unauthorized(ss, req);
        }

        ss << SRS_SIP_VERSION <<" 200 OK" << SRS_RTSP_CRLF
        << "Via: " << req->via << SRS_RTSP_CRLF
        << "From: <sip:"<< req->from << ">" << SRS_RTSP_CRLF
        << "To: <sip:"<< req->to << ">" << SRS_RTSP_CRLF
        << "CSeq: "<< req->seq << " " << req->method <<  SRS_RTSP_CRLF
        << "Call-ID: " << req->call_id << SRS_RTSP_CRLF
        << "Contact: " << req->contact << SRS_RTSP_CRLF
        << "User-Agent: " << SRS_SIP_USER_AGENT << SRS_RTSP_CRLF
        << "Expires: " << req->expires << SRS_RTSP_CRLF
        << "Content-Length: 0" << SRS_RTSP_CRLFCRLF;
    }else{
        /*
        //request: sip-agnet-------MESSAGE------->sip-server
        MESSAGE sip:34020000002000000001@3402000000 SIP/2.0
        Via: SIP/2.0/UDP 192.168.137.11:5060;rport;branch=z9hG4bK1066375804
        From: <sip:34020000001320000003@3402000000>;tag=1925919231
        To: <sip:34020000002000000001@3402000000>
        Call-ID: 1185236415
        CSeq: 20 MESSAGE
        Content-Type: Application/MANSCDP+xml
        Max-Forwards: 70
        User-Agent: IP Camera
        Content-Length:   175

        <?xml version="1.0" encoding="UTF-8"?>
        <Notify>
        <CmdType>Keepalive</CmdType>
        <SN>1</SN>
        <DeviceID>34020000001320000003</DeviceID>
        <Status>OK</Status>
        <Info>
        </Info>
        </Notify>
        //response: sip-agent------200 OK --------> sip-server
        SIP/2.0 200 OK
        Via: SIP/2.0/UDP 192.168.137.11:5060;rport;branch=z9hG4bK1066375804
        From: <sip:34020000001320000003@3402000000>
        To: <sip:34020000002000000001@3402000000>
        CSeq: 20 MESSAGE
        Call-ID: 1185236415
        User-Agent: SRS/4.0.4(Leo)
        Content-Length: 0
        
        */

        ss << SRS_SIP_VERSION <<" 200 OK" << SRS_RTSP_CRLF
        << "Via: " << req->via << SRS_RTSP_CRLF
        << "From: <sip:"<< req->from << ">" << SRS_RTSP_CRLF
        << "To: <sip:"<< req->to << ">" << SRS_RTSP_CRLF
        << "CSeq: "<< req->seq << " " << req->method <<  SRS_RTSP_CRLF
        << "Call-ID: " << req->call_id << SRS_RTSP_CRLF
        << "User-Agent: " << SRS_SIP_USER_AGENT << SRS_RTSP_CRLF
        << "Content-Length: 0" << SRS_RTSP_CRLFCRLF;
    }
   
}

void SrsSipStack::req_invite(stringstream& ss, SrsSipRequest *req, string ip, int port, uint32_t ssrc)
{
    /* 
    //request: sip-agent <-------INVITE------ sip-server
    INVITE sip:34020000001320000003@3402000000 SIP/2.0
    Via: SIP/2.0/UDP 39.100.155.146:15063;rport;branch=z9hG4bK34208805
    From: <sip:34020000002000000001@39.100.155.146:15063>;tag=512358805
    To: <sip:34020000001320000003@3402000000>
    Call-ID: 200008805
    CSeq: 20 INVITE
    Content-Type: Application/SDP
    Contact: <sip:34020000001320000003@3402000000>
    Max-Forwards: 70 
    User-Agent: SRS/4.0.4(Leo)
    Subject: 34020000001320000003:630886,34020000002000000001:0
    Content-Length: 164

    v=0
    o=34020000001320000003 0 0 IN IP4 39.100.155.146
    s=Play
    c=IN IP4 39.100.155.146
    t=0 0
    m=video 9000 RTP/AVP 96
    a=recvonly
    a=rtpmap:96 PS/90000
    y=630886
    //response: sip-agent --------100 Trying--------> sip-server
    SIP/2.0 100 Trying
    Via: SIP/2.0/UDP 39.100.155.146:15063;rport=15063;branch=z9hG4bK34208805
    From: <sip:34020000002000000001@39.100.155.146:15063>;tag=512358805
    To: <sip:34020000001320000003@3402000000>
    Call-ID: 200008805
    CSeq: 20 INVITE
    User-Agent: IP Camera
    Content-Length: 0

    //response: sip-agent -------200 OK--------> sip-server 
    SIP/2.0 200 OK
    Via: SIP/2.0/UDP 39.100.155.146:15063;rport=15063;branch=z9hG4bK34208805
    From: <sip:34020000002000000001@39.100.155.146:15063>;tag=512358805
    To: <sip:34020000001320000003@3402000000>;tag=1083111311
    Call-ID: 200008805
    CSeq: 20 INVITE
    Contact: <sip:34020000001320000003@192.168.137.11:5060>
    Content-Type: application/sdp
    User-Agent: IP Camera
    Content-Length:   263

    v=0
    o=34020000001320000003 1073 1073 IN IP4 192.168.137.11
    s=Play
    c=IN IP4 192.168.137.11
    t=0 0
    m=video 15060 RTP/AVP 96
    a=setup:active
    a=sendonly
    a=rtpmap:96 PS/90000
    a=username:34020000001320000003
    a=password:12345678
    a=filesize:0
    y=0000630886
    f=
    //request: sip-agent <------ ACK ------- sip-server
    ACK sip:34020000001320000003@3402000000 SIP/2.0
    Via: SIP/2.0/UDP 39.100.155.146:15063;rport;branch=z9hG4bK34208805
    From: <sip:34020000002000000001@39.100.155.146:15063>;tag=512358805
    To: <sip:34020000001320000003@3402000000>
    Call-ID: 200008805
    CSeq: 20 ACK
    Max-Forwards: 70
    User-Agent: SRS/4.0.4(Leo)
    Content-Length: 0
    */

    std::stringstream sdp;
    sdp << "v=0" << SRS_RTSP_CRLF
    << "o=" << req->sip_auth_id << " 0 0 IN IP4 " << ip << SRS_RTSP_CRLF
    << "s=Play" << SRS_RTSP_CRLF
    << "c=IN IP4 " << ip << SRS_RTSP_CRLF
    << "t=0 0" << SRS_RTSP_CRLF
    //TODO 97 98 99 current no support
    //<< "m=video " << port <<" RTP/AVP 96 97 98 99" << SRS_RTSP_CRLF
    << "m=video " << port <<" RTP/AVP 96" << SRS_RTSP_CRLF
    << "a=recvonly" << SRS_RTSP_CRLF
    << "a=rtpmap:96 PS/90000" << SRS_RTSP_CRLF
    //TODO: current no support
    //<< "a=rtpmap:97 MPEG4/90000" << SRS_RTSP_CRLF
    //<< "a=rtpmap:98 H264/90000" << SRS_RTSP_CRLF
    //<< "a=rtpmap:99 H265/90000" << SRS_RTSP_CRLF
    //<< "a=streamMode:MAIN\r\n"
    //<< "a=filesize:0\r\n"
    << "y=" << ssrc << SRS_RTSP_CRLF;

    
    int rand = srs_sip_random(1000, 9999);
    std::stringstream from, to, uri, branch, from_tag;
    //"INVITE sip:34020000001320000001@3402000000 SIP/2.0\r\n
    uri << "sip:" <<  req->sip_auth_id << "@" << req->realm;
    //From: <sip:34020000002000000001@%s:%s>;tag=500485%d\r\n
    from << req->serial << "@" << req->host << ":"  << req->host_port;
    to << req->sip_auth_id <<  "@" << req->realm;

    req->from = from.str();
    req->to   = to.str();
    req->uri  = uri.str();

    branch << "z9hG4bK3420" << rand;
    from_tag << "51235" << rand;
    req->branch = branch.str();
    req->from_tag = from_tag.str();

    ss << "INVITE " << req->uri << " " << SRS_SIP_VERSION << SRS_RTSP_CRLF
    << "Via: " << SRS_SIP_VERSION << "/UDP "<< req->host << ":" << req->host_port << ";rport;branch=" << req->branch << SRS_RTSP_CRLF
    << "From: <sip:" << req->from << ">;tag=" << req->from_tag << SRS_RTSP_CRLF
    << "To: <sip:" << req->to << ">" << SRS_RTSP_CRLF
    << "Call-ID: 20000" << rand <<SRS_RTSP_CRLF
    << "CSeq: 20 INVITE" << SRS_RTSP_CRLF
    << "Content-Type: Application/SDP" << SRS_RTSP_CRLF
    << "Contact: <sip:" << req->to << ">" << SRS_RTSP_CRLF
    << "Max-Forwards: 70" << " \r\n"
    << "User-Agent: " << SRS_SIP_USER_AGENT <<SRS_RTSP_CRLF
    << "Subject: "<< req->sip_auth_id << ":" << ssrc << "," << req->serial << ":0" << SRS_RTSP_CRLF
    << "Content-Length: " << sdp.str().length() << SRS_RTSP_CRLFCRLF
    << sdp.str();
}


void SrsSipStack::req_401_unauthorized(std::stringstream& ss, SrsSipRequest *req)
{
    /* sip-agent <-----401 Unauthorized ------ sip-server
    SIP/2.0 401 Unauthorized
    Via: SIP/2.0/UDP 192.168.137.92:5061;rport=61378;received=192.168.1.13;branch=z9hG4bK802519080
    From: <sip:34020000001320000004@192.168.137.92:5061>;tag=611442989
    To: <sip:34020000001320000004@192.168.137.92:5061>;tag=102092689
    CSeq: 1 REGISTER
    Call-ID: 1650345118
    User-Agent: LiveGBS v200228
    Contact: <sip:34020000002000000001@192.168.1.23:15060>
    Content-Length: 0
    WWW-Authenticate: Digest realm="3402000000",qop="auth",nonce="f1da98bd160f3e2efe954c6eedf5f75a"
    */

    ss << SRS_SIP_VERSION <<" 401 Unauthorized" << SRS_RTSP_CRLF
    //<< "Via: " << req->via << SRS_RTSP_CRLF
    << "Via: " << req->via << ";rport=" << req->peer_port << ";received=" << req->peer_ip << ";branch=" << req->branch << SRS_RTSP_CRLF
    << "From: <sip:"<< req->from << ">" << SRS_RTSP_CRLF
    << "To: <sip:"<< req->to << ">" << SRS_RTSP_CRLF
    << "CSeq: "<< req->seq << " " << req->method <<  SRS_RTSP_CRLF
    << "Call-ID: " << req->call_id << SRS_RTSP_CRLF
    << "Contact: " << req->contact << SRS_RTSP_CRLF
    << "User-Agent: " << SRS_SIP_USER_AGENT << SRS_RTSP_CRLF
    << "Content-Length: 0" << SRS_RTSP_CRLF
    << "WWW-Authenticate: Digest realm=\"3402000000\",qop=\"auth\",nonce=\"f1da98bd160f3e2efe954c6eedf5f75a\"" << SRS_RTSP_CRLFCRLF;
    return;
}

void SrsSipStack::resp_ack(std::stringstream& ss, SrsSipRequest *req){
    /*
    //request: sip-agent <------ ACK ------- sip-server
    ACK sip:34020000001320000003@3402000000 SIP/2.0
    Via: SIP/2.0/UDP 39.100.155.146:15063;rport;branch=z9hG4bK34208805
    From: <sip:34020000002000000001@39.100.155.146:15063>;tag=512358805
    To: <sip:34020000001320000003@3402000000>
    Call-ID: 200008805
    CSeq: 20 ACK
    Max-Forwards: 70
    User-Agent: SRS/4.0.4(Leo)
    Content-Length: 0
    */
  
    ss << "ACK " << "sip:" <<  req->sip_auth_id << "@" << req->realm << " "<< SRS_SIP_VERSION << SRS_RTSP_CRLF
    << "Via: " << SRS_SIP_VERSION << "/UDP " << req->host << ":" << req->host_port << ";rport;branch=" << req->branch << SRS_RTSP_CRLF
    << "From: <sip:" << req->serial << "@" << req->host + ":" << req->host_port << ">;tag=" << req->from_tag << SRS_RTSP_CRLF
    << "To: <sip:"<< req->sip_auth_id <<  "@" << req->realm << ">\r\n"
    << "Call-ID: " << req->call_id << SRS_RTSP_CRLF
    << "CSeq: " << req->seq << " ACK"<< SRS_RTSP_CRLF
    << "Max-Forwards: 70" << SRS_RTSP_CRLF
    << "User-Agent: "<< SRS_SIP_USER_AGENT << SRS_RTSP_CRLF
    << "Content-Length: 0" << SRS_RTSP_CRLFCRLF;
}

void SrsSipStack::req_bye(std::stringstream& ss, SrsSipRequest *req)
{
    /*
    //request: sip-agent <------BYE------ sip-server
    BYE sip:34020000001320000003@3402000000 SIP/2.0
    Via: SIP/2.0/UDP 39.100.155.146:15063;rport;branch=z9hG4bK34208805
    From: <sip:34020000002000000001@3402000000>;tag=512358805
    To: <sip:34020000001320000003@3402000000>;tag=1083111311
    Call-ID: 200008805
    CSeq: 79 BYE
    Max-Forwards: 70
    User-Agent: SRS/4.0.4(Leo)
    Content-Length: 0

    //response: sip-agent ------200 OK ------> sip-server
    SIP/2.0 200 OK
    Via: SIP/2.0/UDP 39.100.155.146:15063;rport=15063;branch=z9hG4bK34208805
    From: <sip:34020000002000000001@3402000000>;tag=512358805
    To: <sip:34020000001320000003@3402000000>;tag=1083111311
    Call-ID: 200008805
    CSeq: 79 BYE
    User-Agent: IP Camera
    Content-Length: 0

    */

    std::stringstream from, to, uri;
    uri << "sip:" <<  req->sip_auth_id << "@" << req->realm;
    from << req->serial << "@"  << req->realm;
    to << req->sip_auth_id <<  "@" <<  req->realm;

    req->from = from.str();
    req->to   = to.str();
    req->uri  = uri.str();

    string to_tag, from_tag, branch;

    if (req->branch.empty()){
        branch = "";
    }else {
        branch = ";branch=" + req->branch;
    }

    if (req->from_tag.empty()){
        from_tag = "";
    }else {
        from_tag = ";tag=" + req->from_tag;
    }
    
    if (req->to_tag.empty()){
        to_tag = "";
    }else {
        to_tag = ";tag=" + req->to_tag;
    }

    int seq = srs_sip_random(22, 99);
    ss << "BYE " << req->uri << " "<< SRS_SIP_VERSION << SRS_RTSP_CRLF
    << "Via: "<< SRS_SIP_VERSION << "/UDP "<< req->host << ":" << req->host_port << ";rport" << branch << SRS_RTSP_CRLF
    << "From: <sip:" << req->from << ">" << from_tag << SRS_RTSP_CRLF
    << "To: <sip:" << req->to << ">" << to_tag << SRS_RTSP_CRLF
    << "Call-ID: " << req->call_id << SRS_RTSP_CRLF
    << "CSeq: "<< seq <<" BYE" << SRS_RTSP_CRLF
    << "Max-Forwards: 70" << SRS_RTSP_CRLF
    << "User-Agent: " << SRS_SIP_USER_AGENT << SRS_RTSP_CRLF
    << "Content-Length: 0" << SRS_RTSP_CRLFCRLF;
   
}

#endif


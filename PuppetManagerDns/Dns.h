#pragma once
#include <vector>
#include <string>
#include <map>

#include "Net.h"

struct DNSHeader {
    uint16_t id;
    uint16_t flags;
    uint16_t questions;
    uint16_t answers;
    uint16_t authority;
    uint16_t additional;
};

struct TunnelRequest {
    std::string encodedData;
    uint16_t qtype = 16;   // TXT记录
    uint16_t qclass = 1;   // IN
};

struct TunnelResponse {
    std::string encodedData;
    uint32_t ttl = 300;
};

struct FragmentControl {
    uint16_t totalFrags = 0;
    bool isMultiFragment = false;
};

class DNS {
public:
    DNS(const DNSHeader& header, const TunnelRequest& request, const TunnelResponse& response);

    // 核心功能
    std::vector<uint8_t> buildClientRequest() const;
    std::vector<uint8_t> buildServerResponse() const;
    std::string parseResponse(const std::vector<uint8_t>& response) const;

    // 分片处理
    std::vector<std::vector<uint8_t>> fragmentPayload(const std::string& payload,
        uint16_t maxSize, FragmentControl& ctrl, bool isResponse);
    bool sendFragments(SOCKET sock, const std::vector<std::vector<uint8_t>>& fragments,
        const sockaddr_in& target, FragmentControl& ctrl) const;
    std::map<uint16_t, std::string> receiveFragmentsServer2Client(SOCKET sock, sockaddr_in& expectedAddr) const;
    std::map<uint16_t, std::string> receiveFragmentsClient2Server(SOCKET sock, sockaddr_in& expectedAddr) const;
    std::string assembleFragments(const std::map<uint16_t, std::string>& fragments) const;

    // 工具方法
    std::string extractQueryData(const std::vector<uint8_t>& query) const;
    void sendResponse(SOCKET sock, const sockaddr_in& clientAddr, const std::string& responseData);
    void sendRequest(SOCKET sock, const std::string& payload, const char* serverIP,u_short port);

    // Base64编解码
    static std::string base64Encode(const std::string& input);
    static std::string base64Decode(const std::string& input);

private:
    DNSHeader m_header;
    TunnelRequest m_request;
    TunnelResponse m_response;

    // 私有工具方法
    std::vector<uint8_t> domainEncode(const std::string& input) const;
    std::string domainDecode(const uint8_t* data, size_t length) const;
    uint16_t extractFragmentInfo(const std::string& domain, bool extractTotal) const;
    std::string extractFragmentData(const std::vector<uint8_t>& packet) const;

};
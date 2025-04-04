#include "dns.h"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <map>
#include <random>
#include <thread>  // 支持std::this_thread::sleep_for
#include "AES.h"

// Base64字符集
const std::string BASE64_CHARS =
"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/*------------------------------
  Base64编解码实现
------------------------------*/
std::string DNS::base64Encode(const std::string& input) {
    std::string encoded;
    int i = 0, j = 0;
    uint8_t char_array_3[3], char_array_4[4];

    for (auto& c : input) {
        char_array_3[i++] = c;
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for (i = 0; i < 4; i++)
                encoded += BASE64_CHARS[char_array_4[i]];
            i = 0;
        }
    }

    if (i) {
        for (j = i; j < 3; j++) char_array_3[j] = '\0';
        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);

        for (j = 0; j < i + 1; j++)
            encoded += BASE64_CHARS[char_array_4[j]];
        while (i++ < 3) encoded += '=';
    }
    return encoded;
}

std::string DNS::base64Decode(const std::string& input) {
    std::string decoded;
    std::vector<int> T(256, -1);
    for (int i = 0; i < 64; i++) T[BASE64_CHARS[i]] = i;

    int i = 0, val = 0;
    for (auto& c : input) {
        if (T[c] == -1) break;
        val = (val << 6) + T[c];
        i += 6;
        if (i >= 8) {
            i -= 8;
            decoded += (char)((val >> i) & 0xFF);
        }
    }
    return decoded;
}

DNS::DNS(const DNSHeader& header, const TunnelRequest& request, const TunnelResponse& response)
{
}

/*------------------------------
  DNS报文构建
------------------------------*/
std::vector<uint8_t> DNS::buildClientRequest() const {
    std::vector<uint8_t> packet;
    DNSHeader netHeader = {
        htons(0x1234),
        htons(0x0100),
        htons(1), 0, 0, 0  // QDCOUNT=1,其他为0
    };

    // 添加头部
    const uint8_t* headerPtr = reinterpret_cast<const uint8_t*>(&netHeader);
    packet.insert(packet.end(), headerPtr, headerPtr + sizeof(DNSHeader));

    // 编码域名
    std::vector<uint8_t> encodedDomain = domainEncode(m_request.encodedData);
    packet.insert(packet.end(), encodedDomain.begin(), encodedDomain.end());

    // 添加查询类型和类
    uint16_t netType = htons(m_request.qtype);
    uint16_t netClass = htons(m_request.qclass);
    packet.insert(packet.end(), reinterpret_cast<const uint8_t*>(&netType),
        reinterpret_cast<const uint8_t*>(&netType) + 2);
    packet.insert(packet.end(), reinterpret_cast<const uint8_t*>(&netClass),
        reinterpret_cast<const uint8_t*>(&netClass) + 2);
    return packet;
}

std::vector<uint8_t> DNS::buildServerResponse() const {
    std::vector<uint8_t> packet;
    DNSHeader netHeader = {
        htons(0x1234),
        htons(0x8180),  // Flags: QR=1, AA=1
        htons(1), htons(1), 0, 0  // QDCOUNT=1, ANCOUNT=1
    };

    // 添加头部
    const uint8_t* headerPtr = reinterpret_cast<const uint8_t*>(&netHeader);
    packet.insert(packet.end(), headerPtr, headerPtr + sizeof(DNSHeader));

    // 添加问题部分
    std::vector<uint8_t> encodedDomain = domainEncode(m_request.encodedData);
    packet.insert(packet.end(), encodedDomain.begin(), encodedDomain.end());
    uint16_t net_qtype = htons(m_request.qtype);
    uint16_t net_qclass = htons(m_request.qclass);
    packet.insert(packet.end(), reinterpret_cast<const uint8_t*>(&net_qtype),
        reinterpret_cast<const uint8_t*>(&net_qtype) + 2);
    packet.insert(packet.end(), reinterpret_cast<const uint8_t*>(&net_qclass),
        reinterpret_cast<const uint8_t*>(&net_qclass) + 2);

    // 添加资源记录
    const uint16_t namePtr = htons(0xC00C);  // 指向问题部分的域名
    packet.insert(packet.end(), reinterpret_cast<const uint8_t*>(&namePtr),
        reinterpret_cast<const uint8_t*>(&namePtr) + 2);

    const uint16_t type = htons(16);   // TXT类型
    const uint16_t qclass = htons(1);  // IN类
    const uint32_t ttl = htonl(m_response.ttl);
    packet.insert(packet.end(), reinterpret_cast<const uint8_t*>(&type),
        reinterpret_cast<const uint8_t*>(&type) + 2);
    packet.insert(packet.end(), reinterpret_cast<const uint8_t*>(&qclass),
        reinterpret_cast<const uint8_t*>(&qclass) + 2);
    packet.insert(packet.end(), reinterpret_cast<const uint8_t*>(&ttl),
        reinterpret_cast<const uint8_t*>(&ttl) + 4);

    // 添加TXT数据
    std::vector<uint8_t> txtData;
    txtData.push_back(static_cast<uint8_t>(m_response.encodedData.size()));
    txtData.insert(txtData.end(), m_response.encodedData.begin(), m_response.encodedData.end());

    const uint16_t dataLen = htons(txtData.size());
    packet.insert(packet.end(), reinterpret_cast<const uint8_t*>(&dataLen),
        reinterpret_cast<const uint8_t*>(&dataLen) + 2);
    packet.insert(packet.end(), txtData.begin(), txtData.end());
    return packet;
}

/*------------------------------
  DNS报文解析
------------------------------*/
std::string DNS::parseResponse(const std::vector<uint8_t>& response) const {
    if (response.size() < sizeof(DNSHeader)) return "";

    const DNSHeader* header = reinterpret_cast<const DNSHeader*>(response.data());
    uint16_t answerCount = ntohs(header->answers);
    if (answerCount == 0) return "";

    size_t offset = sizeof(DNSHeader);
    // 跳过问题部分
    while (offset < response.size() && response[offset] != 0) {
        if ((response[offset] & 0xC0) == 0xC0) {
            offset += 2;
            break;
        }
        uint8_t len = response[offset++];
        offset += len;
    }
    offset += 5;  // 跳过最后的0和QTYPE/QCLASS

    std::string result;
    for (uint16_t i = 0; i < answerCount && offset < response.size(); ++i) {
        // 跳过NAME字段
        if ((response[offset] & 0xC0) == 0xC0) {
            offset += 2;
        }
        else {
            while (offset < response.size() && response[offset] != 0) {
                offset += response[offset] + 1;
            }
            offset++;
        }

        if (offset + 10 > response.size()) break;

        uint16_t type = ntohs(*reinterpret_cast<const uint16_t*>(&response[offset]));
        offset += 2;
        offset += 8;  // 跳过CLASS/TTL/RDLENGTH

        uint16_t dataLen = ntohs(*reinterpret_cast<const uint16_t*>(&response[offset - 2]));
        if (type == 16) {  // TXT记录
            size_t dataEnd = offset + dataLen;
            while (offset < dataEnd) {
                uint8_t segLen = response[offset++];
                if (segLen == 0 || offset + segLen > dataEnd) break;
                result.append(reinterpret_cast<const char*>(&response[offset]), segLen);
                offset += segLen;
            }
        }
        else {
            offset += dataLen;
        }
    }
    return result;
}

/*------------------------------
  分片处理实现
------------------------------*/
std::vector<std::vector<uint8_t>> DNS::fragmentPayload(
    const std::string& payload,
    uint16_t maxSize,
    FragmentControl& ctrl,
    bool isResponse)
{
    std::vector<std::vector<uint8_t>> fragments;
    const size_t MAX_LABEL_LEN = 63;
    const size_t FIXED_OVERHEAD = 24;  // DNS头部+问题部分固定开销

    // Base64编码原始数据
    std::string key = "weiweiSecretKey123!";
    std::string encodedPayload = base64Encode(Encrypt::encodeByAES(payload, key));

    // 计算每个分片的最大有效载荷长度（考虑标签分割的点）
    const size_t maxDataLen_total = (maxSize > FIXED_OVERHEAD) ?
        (maxSize - FIXED_OVERHEAD) : 0;

    // 计算每个分片能容纳的标签块数量
    const size_t max_labels_per_fragment = (maxDataLen_total + 1) / (MAX_LABEL_LEN + 1);
    size_t maxDataLen = max_labels_per_fragment * MAX_LABEL_LEN;
    maxDataLen = std::max<size_t>(maxDataLen, 1);  // 至少1字节

    ctrl.totalFrags = static_cast<uint16_t>(
        (encodedPayload.size() + maxDataLen - 1) / maxDataLen);
    ctrl.isMultiFragment = (ctrl.totalFrags > 1);

    for (uint16_t seq = 0; seq < ctrl.totalFrags; ++seq) {
        // 分片数据切割
        size_t start = seq * maxDataLen;
        size_t end = (((start + maxDataLen) < (encodedPayload.size())) ? (start + maxDataLen) : (encodedPayload.size()));
        std::string fragment = encodedPayload.substr(start, end - start);

        // 构建分片元数据前缀
        std::ostringstream metaPrefix;
        metaPrefix << "f" << seq << "."
            << ctrl.totalFrags << "."
            << (ctrl.isMultiFragment ? "1" : "0");

        if (isResponse) {
            // ====================== 响应分片构建 ======================
            // 问题部分：固定格式 fX.Y.Z.bing.com
            std::string questionDomain = metaPrefix.str() + ".bing.com";

            // 答案部分：直接存储原始数据分片（无需编码标签）
            m_request.encodedData = questionDomain;

            m_response.encodedData = fragment;  // 直接存储数据分片

            fragments.push_back(buildServerResponse());
        }
        else {
            // ====================== 请求分片构建 ======================
            // 将数据分片切割为多个标签
            std::vector<std::string> dataLabels;
            for (size_t offset = 0; offset < fragment.size(); offset += MAX_LABEL_LEN) {
                size_t chunkSize = (((MAX_LABEL_LEN) < (fragment.size() - offset)) ? (MAX_LABEL_LEN) : (fragment.size() - offset));
                dataLabels.push_back(fragment.substr(offset, chunkSize));
            }

            // 构建完整域名：fX.Y.Z.data1.data2...bing.com
            std::ostringstream fullDomain;
            fullDomain << metaPrefix.str();
            for (const auto& label : dataLabels) {
                fullDomain << "." << label;
            }
            fullDomain << ".bing.com";

            // 验证域名总长度
            if (fullDomain.str().size() > 253) {
                throw std::runtime_error("Domain exceeds 253 characters");
            }

            // 构建请求报文
            m_request.encodedData = fullDomain.str();
            fragments.push_back(buildClientRequest());
        }
    }

    return fragments;
}


bool DNS::sendFragments(SOCKET sock,
    const std::vector<std::vector<uint8_t>>& fragments,
    const sockaddr_in& target,
    FragmentControl& ctrl) const
{
    const int MAX_RETRIES = 5;
    const int WINDOW_SIZE = 3;
    const auto TIMEOUT = std::chrono::seconds(2);

    std::vector<bool> ackStatus(fragments.size(), false);
    int retryCount = 0;
    size_t nextToSend = 0;

    // 设置为非阻塞模式
    u_long originalMode;
    ioctlsocket(sock, FIONBIO, &originalMode);
    u_long nonBlocking = 1;
    ioctlsocket(sock, FIONBIO, &nonBlocking);

    while (retryCount < MAX_RETRIES) {
        // 发送窗口内未确认的分片
        size_t windowEnd = (((nextToSend + WINDOW_SIZE) < (fragments.size())) ? (nextToSend + WINDOW_SIZE) : (fragments.size()));
        for (size_t i = nextToSend; i < windowEnd; ++i) {
            if (!ackStatus[i]) {
                sendto(sock, (const char*)fragments[i].data(), fragments[i].size(), 0,
                    (const sockaddr*)&target, sizeof(target));
                std::cout << "[DEBUG] Sent fragment " << i << std::endl;
            }
        }

        // 接收ACK（非阻塞模式）
        auto startTime = std::chrono::steady_clock::now();
        while (std::chrono::steady_clock::now() - startTime < TIMEOUT) {
            uint16_t ackSeq;
            sockaddr_in fromAddr;
            int fromLen = sizeof(fromAddr);
            int bytesReceived = recvfrom(sock, (char*)&ackSeq, sizeof(ackSeq), 0,
                (sockaddr*)&fromAddr, &fromLen);

            // 验证ACK来源
            if (bytesReceived == sizeof(uint16_t)) {
                if (fromAddr.sin_addr.s_addr == target.sin_addr.s_addr &&
                    fromAddr.sin_port == target.sin_port)
                {
                    uint16_t seq = ntohs(ackSeq); // 转换为主机字节序
                    if (seq < fragments.size()) {
                        ackStatus[seq] = true;
                        std::cout << "[DEBUG] Received ACK for fragment " << seq << std::endl;
                        // 推进窗口到第一个未确认的分片
                        while (nextToSend < fragments.size() && ackStatus[nextToSend]) {
                            nextToSend++;
                        }
                    }
                }
            }

            // 检查是否所有分片已确认
            if (nextToSend >= fragments.size()) {
                ioctlsocket(sock, FIONBIO, &originalMode); // 恢复原始模式
                return true;
            }

            // 非阻塞模式下短暂等待
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        // 超时后重试未确认的分片
        retryCount++;
        std::cerr << "[WARN] Timeout, retry " << retryCount << std::endl;
    }

    ioctlsocket(sock, FIONBIO, &originalMode); // 恢复原始模式
    return false;
}

/*------------------------------
  工具方法实现
------------------------------*/
std::string DNS::domainDecode(const uint8_t* data, size_t length) const {
    std::string decoded;
    size_t pos = 0;
    size_t jumps = 0;
    const size_t MAX_JUMPS = 5;

    while (pos < length && data[pos] != 0 && jumps < MAX_JUMPS) {
        if ((data[pos] & 0xC0) == 0xC0) {
            if (pos + 1 >= length) break;
            uint16_t offset = ntohs(*reinterpret_cast<const uint16_t*>(data + pos)) & 0x3FFF;
            pos = offset;
            ++jumps;
            continue;
        }

        uint8_t len = data[pos++];
        if (pos + len > length) break;
        decoded.append(reinterpret_cast<const char*>(data + pos), len);
        decoded += '.';
        pos += len;
    }

    if (!decoded.empty() && decoded.back() == '.') decoded.pop_back();
    return decoded;
}

std::string DNS::extractQueryData(const std::vector<uint8_t>& query) const {
    if (query.size() < sizeof(DNSHeader)) return "";

    size_t offset = sizeof(DNSHeader);
    return domainDecode(query.data() + offset, query.size() - offset);
}

uint16_t DNS::extractFragmentInfo(const std::string& domain, bool extractTotal) const {
    std::vector<std::string> parts;
    std::stringstream ss(domain);
    std::string part;

    while (std::getline(ss, part, '.')) {
        parts.push_back(part);
    }

    if (parts.size() < 4 || parts[0].empty() || parts[0][0] != 'f') return 0;

    try {
        if (extractTotal) {
            return static_cast<uint16_t>(std::stoi(parts[1]));
        }
        else {
            return static_cast<uint16_t>(std::stoi(parts[0].substr(1)));
        }
    }
    catch (...) {
        return 0;
    }
}

void DNS::sendResponse(SOCKET sock,
    const sockaddr_in& clientAddr,
    const std::string& responseData)
{
    constexpr uint16_t MAX_FRAGMENT_SIZE = 240;
    FragmentControl ctrl;

    // 使用临时对象生成分片
    DNS tempDNS(m_header, m_request, m_response);
    auto fragments = tempDNS.fragmentPayload(responseData,
        MAX_FRAGMENT_SIZE, ctrl, true);

    if (ctrl.isMultiFragment) {
        sendFragments(sock, fragments, clientAddr, ctrl);
    }
    else {
        sendto(sock,
            reinterpret_cast<const char*>(fragments[0].data()),
            fragments[0].size(), 0,
            reinterpret_cast<const sockaddr*>(&clientAddr),
            sizeof(clientAddr));
    }
}

void DNS::sendRequest(SOCKET sock, const std::string& payload, const char* serverIP,std::string port) {
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(static_cast<u_short>(std::stoi(port)));
    inet_pton(AF_INET, serverIP, &serverAddr.sin_addr);

    FragmentControl ctrl;
    auto fragments = fragmentPayload(payload, 240, ctrl, false);

    // 发送探针报文（仅分片时发送）
    if (ctrl.isMultiFragment) {
        std::ostringstream probe;
        probe << "probe." << ctrl.totalFrags << "." << (ctrl.isMultiFragment ? "1" : "0") << ".bing.com";
        m_request.encodedData = probe.str();
        auto probePacket = buildClientRequest();
        sendto(sock, (const char*)probePacket.data(), probePacket.size(), 0,
            (const sockaddr*)&serverAddr, sizeof(serverAddr));
    }

    // 发送分片或单次请求
    if (ctrl.isMultiFragment) {
        if (!sendFragments(sock, fragments, serverAddr, ctrl)) {
            throw std::runtime_error("Fragment transmission failed");
        }
    }
    else {
        sendto(sock, (const char*)fragments[0].data(), fragments[0].size(), 0,
            (sockaddr*)&serverAddr, sizeof(serverAddr));
    }

    // 接收响应
    /*sockaddr_in responseAddr{};
    auto responseFrags = receiveFragmentsServer2Client(sock, responseAddr);
    return assembleFragments(responseFrags);*/
}

std::string DNS::assembleFragments(const std::map<uint16_t, std::string>& fragments) const {
    if (fragments.empty()) return "";

    // 验证分片完整性
    uint16_t expectedTotal = fragments.begin()->second.size() > 0 ?
        extractFragmentInfo(fragments.begin()->second, true) : 0;

    if (fragments.size() < expectedTotal) {
        throw std::runtime_error("Missing fragments");
    }

    // 组装数据
    std::string encodedData;
    for (const auto& [seq, data] : fragments) {
        encodedData += data;
    }
    std::string key = "weiweiSecretKey123!";
    return Encrypt::decodeByAES(base64Decode(encodedData), key);
}

std::vector<uint8_t> DNS::domainEncode(const std::string& input) const {
    std::vector<uint8_t> encoded;
    size_t pos = 0;
    while (pos < input.size()) {
        size_t chunk = std::min<size_t>(input.size() - pos, 63);
        encoded.push_back(static_cast<uint8_t>(chunk));
        encoded.insert(encoded.end(), input.begin() + pos, input.begin() + pos + chunk);
        pos += chunk;
    }
    encoded.push_back(0);  // 添加结束标记
    return encoded;
}

/*------------------------------
  分片数据提取实现
------------------------------*/
std::string DNS::extractFragmentData(const std::vector<uint8_t>& packet) const {
    if (packet.size() < sizeof(DNSHeader)) return "";
    const DNSHeader* header = reinterpret_cast<const DNSHeader*>(packet.data());
    bool isResponse = (ntohs(header->flags) & 0x8000) != 0;

    // 处理响应报文
    if (isResponse) {
        return parseResponse(packet);
    }
    // 处理请求报文
    else {
        std::string domain = extractQueryData(packet);
        std::vector<std::string> parts;
        std::istringstream iss(domain);
        std::string part;

        // 分割域名各部分
        while (std::getline(iss, part, '.')) {
            parts.push_back(part);
        }

        // 验证域名格式: fX.Y.Z.data.bing.com
        if (parts.size() >= 5 &&
            parts[parts.size() - 2] == "bing" &&
            parts.back() == "com")
        {
            std::string encodedData;
            // 合并分片数据部分 (从第3部分到倒数第3部分)
            for (size_t i = 3; i < parts.size() - 2; ++i) {
                if (!encodedData.empty()) encodedData += ".";
                encodedData += parts[i];
            }
            std::string key = "weiweiSecretKey123!";

            return Encrypt::decodeByAES(base64Decode(encodedData), key);
        }
        return "";
    }
}

/*------------------------------
  接收分片函数修正
------------------------------*/
std::map<uint16_t, std::string> DNS::receiveFragmentsServer2Client(
    SOCKET sock, sockaddr_in& expectedAddr) const
{
    std::map<uint16_t, std::string> fragments;
    const auto TIMEOUT = std::chrono::seconds(15);
    auto startTime = std::chrono::steady_clock::now();
    uint16_t expectedTotal = 0;

    // 设置为非阻塞模式
    u_long originalMode = 0;
    ioctlsocket(sock, FIONBIO, &originalMode);
    u_long nonBlocking = 1;
    ioctlsocket(sock, FIONBIO, &nonBlocking);

    while (std::chrono::steady_clock::now() - startTime < TIMEOUT) {
        std::vector<uint8_t> buffer(512);
        sockaddr_in fromAddr;
        int fromLen = sizeof(fromAddr);
        int recvLen = recvfrom(sock, reinterpret_cast<char*>(buffer.data()), buffer.size(), 0,
            reinterpret_cast<sockaddr*>(&fromAddr), &fromLen);

        // 验证来源地址
        if (recvLen <= 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        // 解析DNS头部
        const DNSHeader* header = reinterpret_cast<const DNSHeader*>(buffer.data());
        bool isResponse = (ntohs(header->flags) & 0x8180); // 检查QR标志位

        // 情况1：处理服务端响应分片（答案部分包含数据）
        if (isResponse) {
            size_t offset = sizeof(DNSHeader);

            // 跳过问题部分
            while (offset < buffer.size() && buffer[offset] != 0) {
                if ((buffer[offset] & 0xC0) == 0xC0) { // 处理指针跳转
                    offset += 2;
                    break;
                }
                uint8_t len = buffer[offset++];
                offset += len;
            }
            offset++;//
            offset += 4; // 跳过QTYPE和QCLASS

            // 解析答案部分
            uint16_t answerCount = ntohs(header->answers);
            for (uint16_t i = 0; i < answerCount && offset < buffer.size(); ++i) {
                // 跳过NAME字段（使用指针或标签）
                if ((buffer[offset] & 0xC0) == 0xC0) {
                    offset += 2;
                }
                else {
                    while (offset < buffer.size() && buffer[offset] != 0) {
                        offset += buffer[offset] + 1;
                    }
                    offset++;
                }

                if (offset + 10 > buffer.size()) break;

                // 解析资源记录头
                uint16_t type = ntohs(*reinterpret_cast<const uint16_t*>(buffer.data() + offset));
                offset += 2; // TYPE
                offset += 2; // CLASS
                offset += 4; // TTL
                uint16_t dataLen = ntohs(*reinterpret_cast<const uint16_t*>(buffer.data() + offset));
                offset += 2;

                // 提取TXT记录数据
                if (type == 16 && dataLen > 0) { // TXT类型
                    uint8_t txtLen = buffer[offset++];
                    if (txtLen == 0) continue;
                    std::string data(reinterpret_cast<const char*>(buffer.data() + offset), txtLen);

                    // 从问题部分提取分片元数据
                    std::string qname = extractQueryData(buffer);
                    std::vector<std::string> parts;
                    std::istringstream iss(qname);
                    std::string part;
                    while (std::getline(iss, part, '.')) parts.push_back(part);

                    if (parts.size() >= 3 && parts[0][0] == 'f') {
                        try {
                            uint16_t seq = static_cast<uint16_t>(std::stoi(parts[0].substr(1)));
                            expectedTotal = static_cast<uint16_t>(std::stoi(parts[1]));
                            fragments[seq] = data;

                            // 发送ACK
                            uint16_t ack = htons(seq);
                            sendto(sock, reinterpret_cast<const char*>(&ack), sizeof(ack), 0,
                                reinterpret_cast<sockaddr*>(&fromAddr), sizeof(fromAddr));
                        }
                        catch (...) {
                            std::cerr << "[ERROR] Invalid fragment metadata" << std::endl;
                        }
                    }
                }
                offset += dataLen;
            }
        }
        // 情况2：处理客户端请求分片（问题部分包含数据）
        else {
            std::string query = extractQueryData(buffer);
            std::vector<std::string> parts;
            std::istringstream iss(query);
            std::string part;
            while (std::getline(iss, part, '.')) parts.push_back(part);

            if (parts.size() >= 4 && parts[0][0] == 'f') {
                try {
                    uint16_t seq = static_cast<uint16_t>(std::stoi(parts[0].substr(1)));
                    expectedTotal = static_cast<uint16_t>(std::stoi(parts[1]));

                    // 合并数据标签
                    std::string data;
                    for (size_t i = 3; i < parts.size() - 2; ++i) {
                        data += (i == 3 ? "" : ".") + parts[i];
                    }
                    fragments[seq] = data;

                    // 发送ACK
                    uint16_t ack = htons(seq);
                    sendto(sock, reinterpret_cast<const char*>(&ack), sizeof(ack), 0,
                        reinterpret_cast<sockaddr*>(&fromAddr), sizeof(fromAddr));
                }
                catch (...) {
                    std::cerr << "[ERROR] Invalid fragment format" << std::endl;
                }
            }
        }

        // 检查是否收齐所有分片
        if (expectedTotal > 0 && fragments.size() >= expectedTotal) {
            break;
        }
    }

    // 恢复原始阻塞模式
    u_long restoreMode = originalMode;
    ioctlsocket(sock, FIONBIO, &restoreMode);

    return fragments;
}

std::map<uint16_t, std::string> DNS::receiveFragmentsClient2Server(
    SOCKET sock, sockaddr_in& expectedAddr) const
{
    std::map<uint16_t, std::string> fragments;
    const auto TIMEOUT = std::chrono::seconds(5);
    auto startTime = std::chrono::steady_clock::now();

    // 设置为非阻塞模式
    u_long originalMode;
    ioctlsocket(sock, FIONBIO, &originalMode);
    u_long nonBlocking = 1;
    ioctlsocket(sock, FIONBIO, &nonBlocking);

    while (std::chrono::steady_clock::now() - startTime < TIMEOUT) {
        std::vector<uint8_t> buffer(512);
        sockaddr_in fromAddr;
        int fromLen = sizeof(fromAddr);
        int recvLen = recvfrom(sock, (char*)buffer.data(), buffer.size(), 0,
            (sockaddr*)&fromAddr, &fromLen);

        // 仅处理目标客户端的报文
        if (recvLen > 0 &&
            fromAddr.sin_addr.s_addr == expectedAddr.sin_addr.s_addr &&
            fromAddr.sin_port == expectedAddr.sin_port)
        {
            std::string query = extractQueryData(buffer);
            std::vector<std::string> parts;
            std::istringstream iss(query);
            std::string part;
            while (std::getline(iss, part, '.')) parts.push_back(part);

            // 提取分片序号和数据
            if (parts.size() >= 4 && parts[0][0] == 'f') {
                try {
                    uint16_t seq = static_cast<uint16_t>(std::stoi(parts[0].substr(1)));
                    std::string data;
                    for (size_t i = 3; i < parts.size() - 2; ++i) {
                        data += (i == 3 ? "" : ".") + parts[i];
                    }
                    fragments[seq] = data;

                    // 发送ACK（网络字节序）
                    uint16_t ack = htons(seq);
                    sendto(sock, (const char*)&ack, sizeof(ack), 0,
                        (sockaddr*)&fromAddr, sizeof(fromAddr));
                    std::cout << "[DEBUG] Sent ACK for fragment " << seq << std::endl;
                }
                catch (...) {
                    std::cerr << "[ERROR] Invalid fragment format." << std::endl;
                }
            }
        }
        else {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        // 检查是否已收齐所有分片（假设探针中已告知总片数）
        // 实际需结合全局状态跟踪，此处简化为超时返回
    }

    // 恢复原始阻塞模式
    u_long restoreMode = originalMode;
    ioctlsocket(sock, FIONBIO, &restoreMode);

    return fragments;
}
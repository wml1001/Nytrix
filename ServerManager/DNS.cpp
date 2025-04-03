#include "dns.h"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <map>
#include <random>
#include <thread>  // ֧��std::this_thread::sleep_for
#include "AES.h"

// Base64�ַ���
const std::string BASE64_CHARS =
"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/*------------------------------
  Base64�����ʵ��
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
  DNS���Ĺ���
------------------------------*/
std::vector<uint8_t> DNS::buildClientRequest() const {
    std::vector<uint8_t> packet;
    DNSHeader netHeader = {
        htons(0x1234),
        htons(0x0100),
        htons(1), 0, 0, 0  // QDCOUNT=1,����Ϊ0
    };

    // ���ͷ��
    const uint8_t* headerPtr = reinterpret_cast<const uint8_t*>(&netHeader);
    packet.insert(packet.end(), headerPtr, headerPtr + sizeof(DNSHeader));

    // ��������
    std::vector<uint8_t> encodedDomain = domainEncode(m_request.encodedData);
    packet.insert(packet.end(), encodedDomain.begin(), encodedDomain.end());

    // ��Ӳ�ѯ���ͺ���
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

    // ���ͷ��
    const uint8_t* headerPtr = reinterpret_cast<const uint8_t*>(&netHeader);
    packet.insert(packet.end(), headerPtr, headerPtr + sizeof(DNSHeader));

    // ������ⲿ��
    std::vector<uint8_t> encodedDomain = domainEncode(m_request.encodedData);
    packet.insert(packet.end(), encodedDomain.begin(), encodedDomain.end());
    uint16_t net_qtype = htons(m_request.qtype);
    uint16_t net_qclass = htons(m_request.qclass);
    packet.insert(packet.end(), reinterpret_cast<const uint8_t*>(&net_qtype),
        reinterpret_cast<const uint8_t*>(&net_qtype) + 2);
    packet.insert(packet.end(), reinterpret_cast<const uint8_t*>(&net_qclass),
        reinterpret_cast<const uint8_t*>(&net_qclass) + 2);

    // �����Դ��¼
    const uint16_t namePtr = htons(0xC00C);  // ָ�����ⲿ�ֵ�����
    packet.insert(packet.end(), reinterpret_cast<const uint8_t*>(&namePtr),
        reinterpret_cast<const uint8_t*>(&namePtr) + 2);

    const uint16_t type = htons(16);   // TXT����
    const uint16_t qclass = htons(1);  // IN��
    const uint32_t ttl = htonl(m_response.ttl);
    packet.insert(packet.end(), reinterpret_cast<const uint8_t*>(&type),
        reinterpret_cast<const uint8_t*>(&type) + 2);
    packet.insert(packet.end(), reinterpret_cast<const uint8_t*>(&qclass),
        reinterpret_cast<const uint8_t*>(&qclass) + 2);
    packet.insert(packet.end(), reinterpret_cast<const uint8_t*>(&ttl),
        reinterpret_cast<const uint8_t*>(&ttl) + 4);

    // ���TXT����
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
  DNS���Ľ���
------------------------------*/
std::string DNS::parseResponse(const std::vector<uint8_t>& response) const {
    if (response.size() < sizeof(DNSHeader)) return "";

    const DNSHeader* header = reinterpret_cast<const DNSHeader*>(response.data());
    uint16_t answerCount = ntohs(header->answers);
    if (answerCount == 0) return "";

    size_t offset = sizeof(DNSHeader);
    // �������ⲿ��
    while (offset < response.size() && response[offset] != 0) {
        if ((response[offset] & 0xC0) == 0xC0) {
            offset += 2;
            break;
        }
        uint8_t len = response[offset++];
        offset += len;
    }
    offset += 5;  // ��������0��QTYPE/QCLASS

    std::string result;
    for (uint16_t i = 0; i < answerCount && offset < response.size(); ++i) {
        // ����NAME�ֶ�
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
        offset += 8;  // ����CLASS/TTL/RDLENGTH

        uint16_t dataLen = ntohs(*reinterpret_cast<const uint16_t*>(&response[offset - 2]));
        if (type == 16) {  // TXT��¼
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
  ��Ƭ����ʵ��
------------------------------*/
std::vector<std::vector<uint8_t>> DNS::fragmentPayload(
    const std::string& payload,
    uint16_t maxSize,
    FragmentControl& ctrl,
    bool isResponse)
{
    std::vector<std::vector<uint8_t>> fragments;
    const size_t MAX_LABEL_LEN = 63;
    const size_t FIXED_OVERHEAD = 24;  // DNSͷ��+���ⲿ�̶ֹ�����

    // Base64����ԭʼ����
    std::string key = "weiweiSecretKey123!";
    std::string encodedPayload = base64Encode(Encrypt::encodeByAES(payload, key));

    // ����ÿ����Ƭ�������Ч�غɳ��ȣ����Ǳ�ǩ�ָ�ĵ㣩
    const size_t maxDataLen_total = (maxSize > FIXED_OVERHEAD) ?
        (maxSize - FIXED_OVERHEAD) : 0;

    // ����ÿ����Ƭ�����ɵı�ǩ������
    const size_t max_labels_per_fragment = (maxDataLen_total + 1) / (MAX_LABEL_LEN + 1);
    size_t maxDataLen = max_labels_per_fragment * MAX_LABEL_LEN;
    maxDataLen = std::max<size_t>(maxDataLen, 1);  // ����1�ֽ�

    ctrl.totalFrags = static_cast<uint16_t>(
        (encodedPayload.size() + maxDataLen - 1) / maxDataLen);
    ctrl.isMultiFragment = (ctrl.totalFrags > 1);

    for (uint16_t seq = 0; seq < ctrl.totalFrags; ++seq) {
        // ��Ƭ�����и�
        size_t start = seq * maxDataLen;
        size_t end = (((start + maxDataLen) < (encodedPayload.size())) ? (start + maxDataLen) : (encodedPayload.size()));
        std::string fragment = encodedPayload.substr(start, end - start);

        // ������ƬԪ����ǰ׺
        std::ostringstream metaPrefix;
        metaPrefix << "f" << seq << "."
            << ctrl.totalFrags << "."
            << (ctrl.isMultiFragment ? "1" : "0");

        if (isResponse) {
            // ====================== ��Ӧ��Ƭ���� ======================
            // ���ⲿ�֣��̶���ʽ fX.Y.Z.bing.com
            std::string questionDomain = metaPrefix.str() + ".bing.com";

            // �𰸲��֣�ֱ�Ӵ洢ԭʼ���ݷ�Ƭ����������ǩ��
            m_request.encodedData = questionDomain;

            m_response.encodedData = fragment;  // ֱ�Ӵ洢���ݷ�Ƭ

            fragments.push_back(buildServerResponse());
        }
        else {
            // ====================== �����Ƭ���� ======================
            // �����ݷ�Ƭ�и�Ϊ�����ǩ
            std::vector<std::string> dataLabels;
            for (size_t offset = 0; offset < fragment.size(); offset += MAX_LABEL_LEN) {
                size_t chunkSize = (((MAX_LABEL_LEN) < (fragment.size() - offset)) ? (MAX_LABEL_LEN) : (fragment.size() - offset));
                dataLabels.push_back(fragment.substr(offset, chunkSize));
            }

            // ��������������fX.Y.Z.data1.data2...bing.com
            std::ostringstream fullDomain;
            fullDomain << metaPrefix.str();
            for (const auto& label : dataLabels) {
                fullDomain << "." << label;
            }
            fullDomain << ".bing.com";

            // ��֤�����ܳ���
            if (fullDomain.str().size() > 253) {
                throw std::runtime_error("Domain exceeds 253 characters");
            }

            // ����������
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

    // ����Ϊ������ģʽ
    u_long originalMode;
    ioctlsocket(sock, FIONBIO, &originalMode);
    u_long nonBlocking = 1;
    ioctlsocket(sock, FIONBIO, &nonBlocking);

    while (retryCount < MAX_RETRIES) {
        // ���ʹ�����δȷ�ϵķ�Ƭ
        size_t windowEnd = (((nextToSend + WINDOW_SIZE) < (fragments.size())) ? (nextToSend + WINDOW_SIZE) : (fragments.size()));
        for (size_t i = nextToSend; i < windowEnd; ++i) {
            if (!ackStatus[i]) {
                sendto(sock, (const char*)fragments[i].data(), fragments[i].size(), 0,
                    (const sockaddr*)&target, sizeof(target));
                std::cout << "[DEBUG] Sent fragment " << i << std::endl;
            }
        }

        // ����ACK��������ģʽ��
        auto startTime = std::chrono::steady_clock::now();
        while (std::chrono::steady_clock::now() - startTime < TIMEOUT) {
            uint16_t ackSeq;
            sockaddr_in fromAddr;
            int fromLen = sizeof(fromAddr);
            int bytesReceived = recvfrom(sock, (char*)&ackSeq, sizeof(ackSeq), 0,
                (sockaddr*)&fromAddr, &fromLen);

            // ��֤ACK��Դ
            if (bytesReceived == sizeof(uint16_t)) {
                if (fromAddr.sin_addr.s_addr == target.sin_addr.s_addr &&
                    fromAddr.sin_port == target.sin_port)
                {
                    uint16_t seq = ntohs(ackSeq); // ת��Ϊ�����ֽ���
                    if (seq < fragments.size()) {
                        ackStatus[seq] = true;
                        std::cout << "[DEBUG] Received ACK for fragment " << seq << std::endl;
                        // �ƽ����ڵ���һ��δȷ�ϵķ�Ƭ
                        while (nextToSend < fragments.size() && ackStatus[nextToSend]) {
                            nextToSend++;
                        }
                    }
                }
            }

            // ����Ƿ����з�Ƭ��ȷ��
            if (nextToSend >= fragments.size()) {
                ioctlsocket(sock, FIONBIO, &originalMode); // �ָ�ԭʼģʽ
                return true;
            }

            // ������ģʽ�¶��ݵȴ�
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        // ��ʱ������δȷ�ϵķ�Ƭ
        retryCount++;
        std::cerr << "[WARN] Timeout, retry " << retryCount << std::endl;
    }

    ioctlsocket(sock, FIONBIO, &originalMode); // �ָ�ԭʼģʽ
    return false;
}

/*------------------------------
  ���߷���ʵ��
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

    // ʹ����ʱ�������ɷ�Ƭ
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

    // ����̽�뱨�ģ�����Ƭʱ���ͣ�
    if (ctrl.isMultiFragment) {
        std::ostringstream probe;
        probe << "probe." << ctrl.totalFrags << "." << (ctrl.isMultiFragment ? "1" : "0") << ".bing.com";
        m_request.encodedData = probe.str();
        auto probePacket = buildClientRequest();
        sendto(sock, (const char*)probePacket.data(), probePacket.size(), 0,
            (const sockaddr*)&serverAddr, sizeof(serverAddr));
    }

    // ���ͷ�Ƭ�򵥴�����
    if (ctrl.isMultiFragment) {
        if (!sendFragments(sock, fragments, serverAddr, ctrl)) {
            throw std::runtime_error("Fragment transmission failed");
        }
    }
    else {
        sendto(sock, (const char*)fragments[0].data(), fragments[0].size(), 0,
            (sockaddr*)&serverAddr, sizeof(serverAddr));
    }

    // ������Ӧ
    /*sockaddr_in responseAddr{};
    auto responseFrags = receiveFragmentsServer2Client(sock, responseAddr);
    return assembleFragments(responseFrags);*/
}

std::string DNS::assembleFragments(const std::map<uint16_t, std::string>& fragments) const {
    if (fragments.empty()) return "";

    // ��֤��Ƭ������
    uint16_t expectedTotal = fragments.begin()->second.size() > 0 ?
        extractFragmentInfo(fragments.begin()->second, true) : 0;

    if (fragments.size() < expectedTotal) {
        throw std::runtime_error("Missing fragments");
    }

    // ��װ����
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
    encoded.push_back(0);  // ��ӽ������
    return encoded;
}

/*------------------------------
  ��Ƭ������ȡʵ��
------------------------------*/
std::string DNS::extractFragmentData(const std::vector<uint8_t>& packet) const {
    if (packet.size() < sizeof(DNSHeader)) return "";
    const DNSHeader* header = reinterpret_cast<const DNSHeader*>(packet.data());
    bool isResponse = (ntohs(header->flags) & 0x8000) != 0;

    // ������Ӧ����
    if (isResponse) {
        return parseResponse(packet);
    }
    // ����������
    else {
        std::string domain = extractQueryData(packet);
        std::vector<std::string> parts;
        std::istringstream iss(domain);
        std::string part;

        // �ָ�����������
        while (std::getline(iss, part, '.')) {
            parts.push_back(part);
        }

        // ��֤������ʽ: fX.Y.Z.data.bing.com
        if (parts.size() >= 5 &&
            parts[parts.size() - 2] == "bing" &&
            parts.back() == "com")
        {
            std::string encodedData;
            // �ϲ���Ƭ���ݲ��� (�ӵ�3���ֵ�������3����)
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
  ���շ�Ƭ��������
------------------------------*/
std::map<uint16_t, std::string> DNS::receiveFragmentsServer2Client(
    SOCKET sock, sockaddr_in& expectedAddr) const
{
    std::map<uint16_t, std::string> fragments;
    const auto TIMEOUT = std::chrono::seconds(15);
    auto startTime = std::chrono::steady_clock::now();
    uint16_t expectedTotal = 0;

    // ����Ϊ������ģʽ
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

        // ��֤��Դ��ַ
        if (recvLen <= 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        // ����DNSͷ��
        const DNSHeader* header = reinterpret_cast<const DNSHeader*>(buffer.data());
        bool isResponse = (ntohs(header->flags) & 0x8180); // ���QR��־λ

        // ���1������������Ӧ��Ƭ���𰸲��ְ������ݣ�
        if (isResponse) {
            size_t offset = sizeof(DNSHeader);

            // �������ⲿ��
            while (offset < buffer.size() && buffer[offset] != 0) {
                if ((buffer[offset] & 0xC0) == 0xC0) { // ����ָ����ת
                    offset += 2;
                    break;
                }
                uint8_t len = buffer[offset++];
                offset += len;
            }
            offset++;//
            offset += 4; // ����QTYPE��QCLASS

            // �����𰸲���
            uint16_t answerCount = ntohs(header->answers);
            for (uint16_t i = 0; i < answerCount && offset < buffer.size(); ++i) {
                // ����NAME�ֶΣ�ʹ��ָ����ǩ��
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

                // ������Դ��¼ͷ
                uint16_t type = ntohs(*reinterpret_cast<const uint16_t*>(buffer.data() + offset));
                offset += 2; // TYPE
                offset += 2; // CLASS
                offset += 4; // TTL
                uint16_t dataLen = ntohs(*reinterpret_cast<const uint16_t*>(buffer.data() + offset));
                offset += 2;

                // ��ȡTXT��¼����
                if (type == 16 && dataLen > 0) { // TXT����
                    uint8_t txtLen = buffer[offset++];
                    if (txtLen == 0) continue;
                    std::string data(reinterpret_cast<const char*>(buffer.data() + offset), txtLen);

                    // �����ⲿ����ȡ��ƬԪ����
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

                            // ����ACK
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
        // ���2������ͻ��������Ƭ�����ⲿ�ְ������ݣ�
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

                    // �ϲ����ݱ�ǩ
                    std::string data;
                    for (size_t i = 3; i < parts.size() - 2; ++i) {
                        data += (i == 3 ? "" : ".") + parts[i];
                    }
                    fragments[seq] = data;

                    // ����ACK
                    uint16_t ack = htons(seq);
                    sendto(sock, reinterpret_cast<const char*>(&ack), sizeof(ack), 0,
                        reinterpret_cast<sockaddr*>(&fromAddr), sizeof(fromAddr));
                }
                catch (...) {
                    std::cerr << "[ERROR] Invalid fragment format" << std::endl;
                }
            }
        }

        // ����Ƿ��������з�Ƭ
        if (expectedTotal > 0 && fragments.size() >= expectedTotal) {
            break;
        }
    }

    // �ָ�ԭʼ����ģʽ
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

    // ����Ϊ������ģʽ
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

        // ������Ŀ��ͻ��˵ı���
        if (recvLen > 0 &&
            fromAddr.sin_addr.s_addr == expectedAddr.sin_addr.s_addr &&
            fromAddr.sin_port == expectedAddr.sin_port)
        {
            std::string query = extractQueryData(buffer);
            std::vector<std::string> parts;
            std::istringstream iss(query);
            std::string part;
            while (std::getline(iss, part, '.')) parts.push_back(part);

            // ��ȡ��Ƭ��ź�����
            if (parts.size() >= 4 && parts[0][0] == 'f') {
                try {
                    uint16_t seq = static_cast<uint16_t>(std::stoi(parts[0].substr(1)));
                    std::string data;
                    for (size_t i = 3; i < parts.size() - 2; ++i) {
                        data += (i == 3 ? "" : ".") + parts[i];
                    }
                    fragments[seq] = data;

                    // ����ACK�������ֽ���
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

        // ����Ƿ����������з�Ƭ������̽�����Ѹ�֪��Ƭ����
        // ʵ������ȫ��״̬���٣��˴���Ϊ��ʱ����
    }

    // �ָ�ԭʼ����ģʽ
    u_long restoreMode = originalMode;
    ioctlsocket(sock, FIONBIO, &restoreMode);

    return fragments;
}
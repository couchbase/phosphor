class TraceLog {
public:
    TraceLog& getInstance();
private:
    struct TLS {
        TraceChunk tc;
    };

    thread_local static TLS tls;

};
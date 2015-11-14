#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "blackhole/extensions/writer.hpp"
#include <blackhole/handler.hpp>
#include <blackhole/logger.hpp>
#include <blackhole/record.hpp>
#include <blackhole/root.hpp>
#include <blackhole/scoped.hpp>

namespace blackhole {
namespace testing {

using ::testing::Invoke;
using ::testing::_;

namespace mock {
namespace {

class handler_t : public ::blackhole::handler_t {
public:
    MOCK_METHOD1(execute, void(const record_t&));
};

}  // namespace
}  // namespace mock

TEST(RootLogger, Log) {
    // Can be initialized with none handlers, does nothing.
    root_logger_t logger({});
    logger.log(0, "GET /porn.png HTTP/1.1");
}

TEST(RootLogger, DispatchRecordToHandlers) {
    std::vector<std::unique_ptr<handler_t>> handlers;
    std::vector<mock::handler_t*> handlers_view;

    for (int i = 0; i < 4; ++i) {
        std::unique_ptr<mock::handler_t> handler(new mock::handler_t);
        handlers_view.push_back(handler.get());
        handlers.push_back(std::move(handler));
    }

    root_logger_t logger(std::move(handlers));

    for (auto handler : handlers_view) {
        EXPECT_CALL(*handler, execute(_))
            .Times(1)
            .WillOnce(Invoke([](const record_t& record) {
                EXPECT_EQ("GET /porn.png HTTP/1.1", record.message().to_string());
                EXPECT_EQ("GET /porn.png HTTP/1.1", record.formatted().to_string());
                EXPECT_EQ(0, record.severity());
                EXPECT_EQ(0, record.attributes().size());
            }));
    }

    logger.log(0, "GET /porn.png HTTP/1.1");
}

TEST(RootLogger, DispatchRecordWithAttributesToHandlers) {
    std::vector<std::unique_ptr<handler_t>> handlers;
    std::vector<mock::handler_t*> handlers_view;

    for (int i = 0; i < 4; ++i) {
        std::unique_ptr<mock::handler_t> handler(new mock::handler_t);
        handlers_view.push_back(handler.get());
        handlers.push_back(std::move(handler));
    }

    root_logger_t logger(std::move(handlers));

    const view_of<attributes_t>::type attributes{{"key#1", {42}}};
    attribute_pack pack{attributes};

    for (auto handler : handlers_view) {
        EXPECT_CALL(*handler, execute(_))
            .Times(1)
            .WillOnce(Invoke([&](const record_t& record) {
                EXPECT_EQ("GET /porn.png HTTP/1.1", record.message().to_string());
                EXPECT_EQ("GET /porn.png HTTP/1.1", record.formatted().to_string());
                EXPECT_EQ(0, record.severity());
                ASSERT_EQ(1, record.attributes().size());
                EXPECT_EQ(attributes, record.attributes().at(0).get());
            }));
    }

    logger.log(0, "GET /porn.png HTTP/1.1", pack);
}

TEST(RootLogger, DispatchRecordWithFormatterToHandlers) {
    std::vector<std::unique_ptr<handler_t>> handlers;
    std::vector<mock::handler_t*> handlers_view;

    for (int i = 0; i < 4; ++i) {
        std::unique_ptr<mock::handler_t> handler(new mock::handler_t);
        handlers_view.push_back(handler.get());
        handlers.push_back(std::move(handler));
    }

    root_logger_t logger(std::move(handlers));

    const view_of<attributes_t>::type attributes{{"key#1", {42}}};
    attribute_pack pack{attributes};

    for (auto handler : handlers_view) {
        EXPECT_CALL(*handler, execute(_))
            .Times(1)
            .WillOnce(Invoke([&](const record_t& record) {
                EXPECT_EQ("GET /porn.png HTTP/1.1 - {}/{}", record.message().to_string());
                EXPECT_EQ("GET /porn.png HTTP/1.1 - 42/2345", record.formatted().to_string());
                EXPECT_EQ(0, record.severity());
                ASSERT_EQ(1, record.attributes().size());
                EXPECT_EQ(attributes, record.attributes().at(0).get());
            }));
    }

    logger.log(0, "GET /porn.png HTTP/1.1 - {}/{}", pack, [](writer_t& writer) {
        writer.write("GET /porn.png HTTP/1.1 - {}/{}", 42, 2345);
    });
}

TEST(RootLogger, Scoped) {
    std::unique_ptr<mock::handler_t> handler(new mock::handler_t);
    mock::handler_t* view = handler.get();

    std::vector<std::unique_ptr<handler_t>> handlers;
    handlers.push_back(std::move(handler));

    root_logger_t logger(std::move(handlers));
    const auto scoped = logger.scoped({{"key#1", {42}}});

    EXPECT_CALL(*view, execute(_))
        .Times(1)
        .WillOnce(Invoke([](const record_t& record) {
            EXPECT_EQ("GET /porn.png HTTP/1.1", record.message().to_string());
            EXPECT_EQ("GET /porn.png HTTP/1.1", record.formatted().to_string());
            EXPECT_EQ(0, record.severity());
            ASSERT_EQ(1, record.attributes().size());

            view_of<attributes_t>::type attributes{{"key#1", {42}}};
            EXPECT_EQ(attributes, record.attributes().at(0).get());
        }));

    logger.log(0, "GET /porn.png HTTP/1.1");
}

TEST(RootLogger, Assignment) {
    std::unique_ptr<mock::handler_t> handler(new mock::handler_t);
    mock::handler_t* view = handler.get();

    std::vector<std::unique_ptr<handler_t>> handlers;
    handlers.push_back(std::move(handler));

    root_logger_t logger1({});
    root_logger_t logger2(std::move(handlers));
    const auto scoped = logger2.scoped({{"key#1", {42}}});

    // All scoped attributes should be assigned to the new owner.
    logger1 = std::move(logger2);

    EXPECT_CALL(*view, execute(_))
        .Times(1)
        .WillOnce(Invoke([](const record_t& record) {
            EXPECT_EQ("GET /porn.png HTTP/1.1", record.message().to_string());
            EXPECT_EQ("GET /porn.png HTTP/1.1", record.formatted().to_string());
            EXPECT_EQ(0, record.severity());
            ASSERT_EQ(1, record.attributes().size());

            view_of<attributes_t>::type attributes{{"key#1", {42}}};
            EXPECT_EQ(attributes, record.attributes().at(0).get());
        }));

    logger1.log(0, "GET /porn.png HTTP/1.1");
}

}  // namespace testing
}  // namespace blackhole
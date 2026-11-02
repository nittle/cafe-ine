#ifndef LOGIC_H
#define LOGIC_H

#include <cstddef>
#include <map>
#include <optional>
#include <string>
#include <vector>

using userid_t = int;
using itemid_t = int;
using orderid_t = int;
using reviewid_t = int;

// ---------------------------------------------------------------------------
// Menu
// ---------------------------------------------------------------------------

enum class ItemCategory {
    Coffee,
    Tea,
    Drinks,
    Breakfast,
    MainCourse,
    Dessert,
    Snack,
    Other
};

std::string toString(ItemCategory category);

class Item {
public:
    Item() = default;
    Item(itemid_t id,
         std::string name,
         std::string description,
         double price,
         std::string portion,
         std::string imagePath = {},
         ItemCategory category = ItemCategory::Other,
         bool available = true);

    itemid_t getId() const;
    const std::string& getName() const;
    const std::string& getDescription() const;
    double getPrice() const;
    const std::string& getPortion() const;
    const std::string& getImagePath() const;
    ItemCategory getCategory() const;
    bool isAvailable() const;

    void setName(const std::string& name);
    void setDescription(const std::string& description);
    void setPrice(double price);
    void setPortion(const std::string& portion);
    void setImagePath(const std::string& imagePath);
    void setCategory(ItemCategory category);
    void setAvailable(bool available);

    bool operator==(const Item& other) const;

private:
    itemid_t id_ = 0;
    std::string name_;
    std::string description_;
    double price_ = 0.0;
    std::string portion_;
    std::string imagePath_;
    ItemCategory category_ = ItemCategory::Other;
    bool available_ = true;
};

// ---------------------------------------------------------------------------
// Cart
// ---------------------------------------------------------------------------

class CartEntry {
public:
    CartEntry() = default;
    CartEntry(Item item, int quantity = 1);

    const Item& getItem() const;
    int getQuantity() const;
    double getSubtotal() const;

    void setQuantity(int quantity);
    void addQuantity(int amount);

private:
    Item item_;
    int quantity_ = 0;
};

class Cart {
public:
    void addItem(const Item& item, int quantity = 1);
    bool removeItem(itemid_t itemId, int quantity = 1);
    bool setItemQuantity(itemid_t itemId, int quantity);
    void clear();

    bool isEmpty() const;
    bool contains(itemid_t itemId) const;
    int getQuantity(itemid_t itemId) const;
    int getItemCount() const;   // total number of units
    std::size_t getLineCount() const;
    double getTotal() const;

    const std::vector<CartEntry>& getEntries() const;

private:
    std::vector<CartEntry>::iterator findEntry(itemid_t itemId);
    std::vector<CartEntry>::const_iterator findEntry(itemid_t itemId) const;

    std::vector<CartEntry> entries_;
};

// ---------------------------------------------------------------------------
// Delivery and orders
// ---------------------------------------------------------------------------

class DeliveryInfo {
public:
    DeliveryInfo() = default;
    DeliveryInfo(std::string address,
                 std::string phone,
                 std::string recipient = {},
                 std::string comment = {});

    bool isValid() const;

    const std::string& getAddress() const;
    const std::string& getPhone() const;
    const std::string& getRecipient() const;
    const std::string& getComment() const;

    void setAddress(const std::string& address);
    void setPhone(const std::string& phone);
    void setRecipient(const std::string& recipient);
    void setComment(const std::string& comment);

private:
    std::string address_;
    std::string phone_;
    std::string recipient_;
    std::string comment_;
};

enum class OrderStatus {
    Created,
    Preparing,
    Delivering,
    Completed,
    Cancelled
};

std::string toString(OrderStatus status);

class Order {
public:
    Order() = default;
    Order(orderid_t id,
          userid_t userId,
          std::vector<CartEntry> items,
          double itemsTotal,
          double deliveryCost,
          int pointsSpent,
          int pointsEarned,
          DeliveryInfo delivery,
          std::string createdAt);

    orderid_t getId() const;
    userid_t getUserId() const;
    const std::vector<CartEntry>& getItems() const;
    double getItemsTotal() const;
    double getDeliveryCost() const;
    double getTotal() const;  // items + delivery - points discount
    int getPointsSpent() const;
    int getPointsEarned() const;
    const DeliveryInfo& getDelivery() const;
    OrderStatus getStatus() const;
    const std::string& getCreatedAt() const;
    int getItemCount() const;

    void setStatus(OrderStatus status);

private:
    orderid_t id_ = 0;
    userid_t userId_ = 0;
    std::vector<CartEntry> items_;
    double itemsTotal_ = 0.0;
    double deliveryCost_ = 0.0;
    int pointsSpent_ = 0;
    int pointsEarned_ = 0;
    DeliveryInfo delivery_;
    OrderStatus status_ = OrderStatus::Created;
    std::string createdAt_;
};

// ---------------------------------------------------------------------------
// User profile
// ---------------------------------------------------------------------------

class User {
public:
    User() = default;
    User(userid_t id,
         std::string name,
         std::string phone = {},
         std::string address = {},
         int points = 0);

    userid_t getId() const;
    const std::string& getName() const;
    const std::string& getPhone() const;
    const std::string& getAddress() const;
    int getPoints() const;
    const std::vector<orderid_t>& getOrderHistory() const;

    Cart& getCart();
    const Cart& getCart() const;

    void setName(const std::string& name);
    void setPhone(const std::string& phone);
    void setAddress(const std::string& address);
    void addPoints(int points);
    bool spendPoints(int points);
    void addOrderToHistory(orderid_t orderId);

private:
    userid_t id_ = 0;
    std::string name_;
    std::string phone_;
    std::string address_;
    int points_ = 0;
    Cart cart_;
    std::vector<orderid_t> orderHistory_;
};

// ---------------------------------------------------------------------------
// Reviews
// ---------------------------------------------------------------------------

class Review {
public:
    Review() = default;
    Review(reviewid_t id,
           userid_t userId,
           std::string authorName,
           int rating,
           std::string text,
           std::string createdAt);

    reviewid_t getId() const;
    userid_t getUserId() const;
    const std::string& getAuthorName() const;
    int getRating() const;
    const std::string& getText() const;
    const std::string& getCreatedAt() const;

private:
    reviewid_t id_ = 0;
    userid_t userId_ = 0;
    std::string authorName_;
    int rating_ = 0;
    std::string text_;
    std::string createdAt_;
};

// ---------------------------------------------------------------------------
// Application logic facade
// ---------------------------------------------------------------------------

class Logic {
public:
    // 1 point = 1 ruble of discount. A customer earns 1 point per 100 rubles.
    static constexpr int POINTS_PER_100_RUBLES = 1;
    static constexpr double MAX_POINTS_DISCOUNT_SHARE = 0.5;

    Logic();

    // --- Menu -------------------------------------------------------------
    std::vector<Item> getItems() const;
    std::vector<Item> getAvailableItems() const;
    std::optional<Item> findItem(itemid_t itemId) const;
    std::vector<Item> searchItems(const std::string& query) const;
    std::vector<Item> getItemsByCategory(ItemCategory category) const;

    // Returns the id assigned to the new item (0 is replaced by a real id).
    itemid_t addMenuItem(const Item& item);
    bool updateMenuItem(const Item& item);
    bool removeMenuItem(itemid_t itemId);
    bool setItemAvailability(itemid_t itemId, bool available);
    void clearMenu();

    // --- Users ------------------------------------------------------------
    User createUser(const std::string& name,
                    const std::string& phone = {},
                    const std::string& address = {});
    bool hasUser(userid_t userId) const;
    User viewUser(userid_t userId) const;  // throws std::out_of_range
    bool updateUser(const User& user);
    int getUserPoints(userid_t userId) const;
    bool addUserPoints(userid_t userId, int points);

    // --- Cart -------------------------------------------------------------
    Cart viewCart(userid_t userId) const;  // throws std::out_of_range
    void addToUserCart(userid_t userId, Item item);
    bool addToUserCart(userid_t userId, itemid_t itemId, int quantity = 1);
    bool removeFromUserCart(userid_t userId, itemid_t itemId, int quantity = 1);
    bool setCartItemQuantity(userid_t userId, itemid_t itemId, int quantity);
    bool clearUserCart(userid_t userId);

    // --- Orders -----------------------------------------------------------
    double calculateDeliveryCost(userid_t userId) const;
    std::optional<orderid_t> placeOrder(userid_t userId,
                                        const DeliveryInfo& delivery,
                                        int pointsToSpend = 0);
    std::optional<Order> findOrder(orderid_t orderId) const;
    std::vector<Order> getUserOrders(userid_t userId) const;
    std::vector<Order> getAllOrders() const;
    bool updateOrderStatus(orderid_t orderId, OrderStatus status);
    bool cancelOrder(orderid_t orderId);

    // --- Reviews ----------------------------------------------------------
    std::optional<reviewid_t> addReview(userid_t userId,
                                        int rating,
                                        const std::string& text);
    std::vector<Review> getReviews() const;
    double getAverageRating() const;

    // --- Delivery settings ------------------------------------------------
    void setDeliverySettings(double baseCost, double freeFrom);
    double getDeliveryBaseCost() const;
    double getFreeDeliveryFrom() const;

    // Fills the menu with a small demo assortment. Does nothing if the menu
    // is not empty.
    void seedDemoMenu();

private:
    User* findUser(userid_t userId);
    const User* findUser(userid_t userId) const;
    Item* findMenuItem(itemid_t itemId);
    const Item* findMenuItem(itemid_t itemId) const;
    Order* findOrderPtr(orderid_t orderId);

    double deliveryCostForTotal(double itemsTotal) const;

    std::vector<Item> menu_;
    std::map<userid_t, User> users_;
    std::vector<Order> orders_;
    std::vector<Review> reviews_;

    itemid_t nextItemId_ = 1;
    userid_t nextUserId_ = 1;
    orderid_t nextOrderId_ = 1;
    reviewid_t nextReviewId_ = 1;

    double deliveryBaseCost_ = 150.0;
    double freeDeliveryFrom_ = 1000.0;
};

#endif

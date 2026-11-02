#include "logic.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <ctime>
#include <stdexcept>
#include <utility>

namespace {

void requireNotEmpty(const std::string& value, const char* field) {
    if (value.empty()) {
        throw std::invalid_argument(std::string(field) + " must not be empty");
    }
}

void requireNonNegative(double value, const char* field) {
    if (!std::isfinite(value) || value < 0.0) {
        throw std::invalid_argument(std::string(field) + " must be a non-negative number");
    }
}

void requirePositiveQuantity(int quantity) {
    if (quantity <= 0) {
        throw std::invalid_argument("Quantity must be positive");
    }
}

double roundMoney(double value) {
    return std::round(value * 100.0) / 100.0;
}

// Lowercases ASCII and the Russian Cyrillic letters (UTF-8). Other bytes are
// copied unchanged, so the function is safe for any UTF-8 text.
std::string toLowerUtf8(const std::string& value) {
    std::string result;
    result.reserve(value.size());

    for (std::size_t i = 0; i < value.size();) {
        const unsigned char current = static_cast<unsigned char>(value[i]);

        if (current < 0x80) {
            result.push_back(static_cast<char>(std::tolower(current)));
            ++i;
            continue;
        }

        if (current == 0xD0 && i + 1 < value.size()) {
            const unsigned char next = static_cast<unsigned char>(value[i + 1]);

            // А-П -> а-п
            if (next >= 0x90 && next <= 0x9F) {
                result.push_back(static_cast<char>(0xD0));
                result.push_back(static_cast<char>(next + 0x20));
                i += 2;
                continue;
            }

            // Р-Я -> р-я
            if (next >= 0xA0 && next <= 0xAF) {
                result.push_back(static_cast<char>(0xD1));
                result.push_back(static_cast<char>(next - 0x20));
                i += 2;
                continue;
            }

            // Ё -> ё
            if (next == 0x81) {
                result.push_back(static_cast<char>(0xD1));
                result.push_back(static_cast<char>(0x91));
                i += 2;
                continue;
            }
        }

        result.push_back(value[i]);
        ++i;
    }

    return result;
}

std::string nowString() {
    const std::time_t currentTime = std::time(nullptr);
    std::tm localTime{};

#ifdef _WIN32
    localtime_s(&localTime, &currentTime);
#else
    localtime_r(&currentTime, &localTime);
#endif

    char buffer[32] = {};
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", &localTime);
    return buffer;
}

}  // namespace

// ---------------------------------------------------------------------------
// Menu
// ---------------------------------------------------------------------------

std::string toString(ItemCategory category) {
    switch (category) {
        case ItemCategory::Coffee:     return "Кофе";
        case ItemCategory::Tea:        return "Чай";
        case ItemCategory::Drinks:     return "Напитки";
        case ItemCategory::Breakfast:  return "Завтраки";
        case ItemCategory::MainCourse: return "Горячее";
        case ItemCategory::Dessert:    return "Десерты";
        case ItemCategory::Snack:      return "Закуски";
        case ItemCategory::Other:      return "Другое";
    }
    return "Другое";
}

Item::Item(itemid_t id,
           std::string name,
           std::string description,
           double price,
           std::string portion,
           std::string imagePath,
           ItemCategory category,
           bool available)
    : id_(id),
      name_(std::move(name)),
      description_(std::move(description)),
      price_(price),
      portion_(std::move(portion)),
      imagePath_(std::move(imagePath)),
      category_(category),
      available_(available) {
    requireNotEmpty(name_, "Item name");
    requireNonNegative(price_, "Item price");
}

itemid_t Item::getId() const { return id_; }
const std::string& Item::getName() const { return name_; }
const std::string& Item::getDescription() const { return description_; }
double Item::getPrice() const { return price_; }
const std::string& Item::getPortion() const { return portion_; }
const std::string& Item::getImagePath() const { return imagePath_; }
ItemCategory Item::getCategory() const { return category_; }
bool Item::isAvailable() const { return available_; }

void Item::setName(const std::string& name) {
    requireNotEmpty(name, "Item name");
    name_ = name;
}

void Item::setDescription(const std::string& description) { description_ = description; }

void Item::setPrice(double price) {
    requireNonNegative(price, "Item price");
    price_ = price;
}

void Item::setPortion(const std::string& portion) { portion_ = portion; }
void Item::setImagePath(const std::string& imagePath) { imagePath_ = imagePath; }
void Item::setCategory(ItemCategory category) { category_ = category; }
void Item::setAvailable(bool available) { available_ = available; }

bool Item::operator==(const Item& other) const { return id_ == other.id_; }

// ---------------------------------------------------------------------------
// Cart
// ---------------------------------------------------------------------------

CartEntry::CartEntry(Item item, int quantity)
    : item_(std::move(item)), quantity_(quantity) {
    requirePositiveQuantity(quantity_);
}

const Item& CartEntry::getItem() const { return item_; }
int CartEntry::getQuantity() const { return quantity_; }

double CartEntry::getSubtotal() const {
    return roundMoney(item_.getPrice() * static_cast<double>(quantity_));
}

void CartEntry::setQuantity(int quantity) {
    requirePositiveQuantity(quantity);
    quantity_ = quantity;
}

void CartEntry::addQuantity(int amount) {
    if (amount == 0) {
        return;
    }
    if (quantity_ + amount <= 0) {
        throw std::invalid_argument("Quantity must stay positive");
    }
    quantity_ += amount;
}

std::vector<CartEntry>::iterator Cart::findEntry(itemid_t itemId) {
    return std::find_if(entries_.begin(), entries_.end(), [itemId](const CartEntry& entry) {
        return entry.getItem().getId() == itemId;
    });
}

std::vector<CartEntry>::const_iterator Cart::findEntry(itemid_t itemId) const {
    return std::find_if(entries_.begin(), entries_.end(), [itemId](const CartEntry& entry) {
        return entry.getItem().getId() == itemId;
    });
}

void Cart::addItem(const Item& item, int quantity) {
    requirePositiveQuantity(quantity);
    if (item.getId() <= 0) {
        throw std::invalid_argument("Cart item must have a valid id");
    }

    auto entry = findEntry(item.getId());
    if (entry != entries_.end()) {
        entry->addQuantity(quantity);
    } else {
        entries_.emplace_back(item, quantity);
    }
}

bool Cart::removeItem(itemid_t itemId, int quantity) {
    requirePositiveQuantity(quantity);

    auto entry = findEntry(itemId);
    if (entry == entries_.end()) {
        return false;
    }

    if (quantity >= entry->getQuantity()) {
        entries_.erase(entry);
    } else {
        entry->setQuantity(entry->getQuantity() - quantity);
    }
    return true;
}

bool Cart::setItemQuantity(itemid_t itemId, int quantity) {
    if (quantity < 0) {
        throw std::invalid_argument("Quantity must not be negative");
    }

    auto entry = findEntry(itemId);
    if (entry == entries_.end()) {
        return false;
    }

    if (quantity == 0) {
        entries_.erase(entry);
    } else {
        entry->setQuantity(quantity);
    }
    return true;
}

void Cart::clear() { entries_.clear(); }

bool Cart::isEmpty() const { return entries_.empty(); }

bool Cart::contains(itemid_t itemId) const { return findEntry(itemId) != entries_.end(); }

int Cart::getQuantity(itemid_t itemId) const {
    auto entry = findEntry(itemId);
    return entry == entries_.end() ? 0 : entry->getQuantity();
}

int Cart::getItemCount() const {
    int count = 0;
    for (const CartEntry& entry : entries_) {
        count += entry.getQuantity();
    }
    return count;
}

std::size_t Cart::getLineCount() const { return entries_.size(); }

double Cart::getTotal() const {
    double total = 0.0;
    for (const CartEntry& entry : entries_) {
        total += entry.getSubtotal();
    }
    return roundMoney(total);
}

const std::vector<CartEntry>& Cart::getEntries() const { return entries_; }

// ---------------------------------------------------------------------------
// Delivery and orders
// ---------------------------------------------------------------------------

DeliveryInfo::DeliveryInfo(std::string address,
                           std::string phone,
                           std::string recipient,
                           std::string comment)
    : address_(std::move(address)),
      phone_(std::move(phone)),
      recipient_(std::move(recipient)),
      comment_(std::move(comment)) {}

bool DeliveryInfo::isValid() const {
    return !address_.empty() && !phone_.empty();
}

const std::string& DeliveryInfo::getAddress() const { return address_; }
const std::string& DeliveryInfo::getPhone() const { return phone_; }
const std::string& DeliveryInfo::getRecipient() const { return recipient_; }
const std::string& DeliveryInfo::getComment() const { return comment_; }

void DeliveryInfo::setAddress(const std::string& address) { address_ = address; }
void DeliveryInfo::setPhone(const std::string& phone) { phone_ = phone; }
void DeliveryInfo::setRecipient(const std::string& recipient) { recipient_ = recipient; }
void DeliveryInfo::setComment(const std::string& comment) { comment_ = comment; }

std::string toString(OrderStatus status) {
    switch (status) {
        case OrderStatus::Created:    return "Создан";
        case OrderStatus::Preparing:  return "Готовится";
        case OrderStatus::Delivering: return "В доставке";
        case OrderStatus::Completed:  return "Выполнен";
        case OrderStatus::Cancelled:  return "Отменён";
    }
    return "Неизвестно";
}

Order::Order(orderid_t id,
             userid_t userId,
             std::vector<CartEntry> items,
             double itemsTotal,
             double deliveryCost,
             int pointsSpent,
             int pointsEarned,
             DeliveryInfo delivery,
             std::string createdAt)
    : id_(id),
      userId_(userId),
      items_(std::move(items)),
      itemsTotal_(roundMoney(itemsTotal)),
      deliveryCost_(roundMoney(deliveryCost)),
      pointsSpent_(pointsSpent),
      pointsEarned_(pointsEarned),
      delivery_(std::move(delivery)),
      createdAt_(std::move(createdAt)) {}

orderid_t Order::getId() const { return id_; }
userid_t Order::getUserId() const { return userId_; }
const std::vector<CartEntry>& Order::getItems() const { return items_; }
double Order::getItemsTotal() const { return itemsTotal_; }
double Order::getDeliveryCost() const { return deliveryCost_; }

double Order::getTotal() const {
    return roundMoney(itemsTotal_ + deliveryCost_ - static_cast<double>(pointsSpent_));
}

int Order::getPointsSpent() const { return pointsSpent_; }
int Order::getPointsEarned() const { return pointsEarned_; }
const DeliveryInfo& Order::getDelivery() const { return delivery_; }
OrderStatus Order::getStatus() const { return status_; }
const std::string& Order::getCreatedAt() const { return createdAt_; }

int Order::getItemCount() const {
    int count = 0;
    for (const CartEntry& entry : items_) {
        count += entry.getQuantity();
    }
    return count;
}

void Order::setStatus(OrderStatus status) { status_ = status; }

// ---------------------------------------------------------------------------
// User
// ---------------------------------------------------------------------------

User::User(userid_t id,
           std::string name,
           std::string phone,
           std::string address,
           int points)
    : id_(id),
      name_(std::move(name)),
      phone_(std::move(phone)),
      address_(std::move(address)),
      points_(points) {
    requireNotEmpty(name_, "User name");
    if (points_ < 0) {
        throw std::invalid_argument("User points must not be negative");
    }
}

userid_t User::getId() const { return id_; }
const std::string& User::getName() const { return name_; }
const std::string& User::getPhone() const { return phone_; }
const std::string& User::getAddress() const { return address_; }
int User::getPoints() const { return points_; }
const std::vector<orderid_t>& User::getOrderHistory() const { return orderHistory_; }

Cart& User::getCart() { return cart_; }
const Cart& User::getCart() const { return cart_; }

void User::setName(const std::string& name) {
    requireNotEmpty(name, "User name");
    name_ = name;
}

void User::setPhone(const std::string& phone) { phone_ = phone; }
void User::setAddress(const std::string& address) { address_ = address; }

void User::addPoints(int points) {
    if (points < 0) {
        throw std::invalid_argument("Points to add must not be negative");
    }
    points_ += points;
}

bool User::spendPoints(int points) {
    if (points < 0) {
        throw std::invalid_argument("Points to spend must not be negative");
    }
    if (points_ < points) {
        return false;
    }
    points_ -= points;
    return true;
}

void User::addOrderToHistory(orderid_t orderId) {
    if (orderId > 0) {
        orderHistory_.push_back(orderId);
    }
}

// ---------------------------------------------------------------------------
// Reviews
// ---------------------------------------------------------------------------

Review::Review(reviewid_t id,
               userid_t userId,
               std::string authorName,
               int rating,
               std::string text,
               std::string createdAt)
    : id_(id),
      userId_(userId),
      authorName_(std::move(authorName)),
      rating_(rating),
      text_(std::move(text)),
      createdAt_(std::move(createdAt)) {
    requireNotEmpty(authorName_, "Review author");
    if (rating_ < 1 || rating_ > 5) {
        throw std::invalid_argument("Review rating must be between 1 and 5");
    }
}

reviewid_t Review::getId() const { return id_; }
userid_t Review::getUserId() const { return userId_; }
const std::string& Review::getAuthorName() const { return authorName_; }
int Review::getRating() const { return rating_; }
const std::string& Review::getText() const { return text_; }
const std::string& Review::getCreatedAt() const { return createdAt_; }

// ---------------------------------------------------------------------------
// Logic
// ---------------------------------------------------------------------------

Logic::Logic() {
    seedDemoMenu();
}

User* Logic::findUser(userid_t userId) {
    auto it = users_.find(userId);
    return it == users_.end() ? nullptr : &it->second;
}

const User* Logic::findUser(userid_t userId) const {
    auto it = users_.find(userId);
    return it == users_.end() ? nullptr : &it->second;
}

Item* Logic::findMenuItem(itemid_t itemId) {
    auto it = std::find_if(menu_.begin(), menu_.end(), [itemId](const Item& item) {
        return item.getId() == itemId;
    });
    return it == menu_.end() ? nullptr : &(*it);
}

const Item* Logic::findMenuItem(itemid_t itemId) const {
    auto it = std::find_if(menu_.begin(), menu_.end(), [itemId](const Item& item) {
        return item.getId() == itemId;
    });
    return it == menu_.end() ? nullptr : &(*it);
}

Order* Logic::findOrderPtr(orderid_t orderId) {
    auto it = std::find_if(orders_.begin(), orders_.end(), [orderId](const Order& order) {
        return order.getId() == orderId;
    });
    return it == orders_.end() ? nullptr : &(*it);
}

double Logic::deliveryCostForTotal(double itemsTotal) const {
    if (itemsTotal <= 0.0 || itemsTotal >= freeDeliveryFrom_) {
        return 0.0;
    }
    return deliveryBaseCost_;
}

std::vector<Item> Logic::getItems() const { return menu_; }

std::vector<Item> Logic::getAvailableItems() const {
    std::vector<Item> result;
    for (const Item& item : menu_) {
        if (item.isAvailable()) {
            result.push_back(item);
        }
    }
    return result;
}

std::optional<Item> Logic::findItem(itemid_t itemId) const {
    const Item* item = findMenuItem(itemId);
    if (item == nullptr) {
        return std::nullopt;
    }
    return *item;
}

std::vector<Item> Logic::searchItems(const std::string& query) const {
    const std::string needle = toLowerUtf8(query);
    if (needle.empty()) {
        return menu_;
    }

    std::vector<Item> result;
    for (const Item& item : menu_) {
        const bool nameMatches =
            toLowerUtf8(item.getName()).find(needle) != std::string::npos;
        const bool descriptionMatches =
            toLowerUtf8(item.getDescription()).find(needle) != std::string::npos;
        const bool categoryMatches =
            toLowerUtf8(toString(item.getCategory())).find(needle) != std::string::npos;

        if (nameMatches || descriptionMatches || categoryMatches) {
            result.push_back(item);
        }
    }
    return result;
}

std::vector<Item> Logic::getItemsByCategory(ItemCategory category) const {
    std::vector<Item> result;
    for (const Item& item : menu_) {
        if (item.getCategory() == category) {
            result.push_back(item);
        }
    }
    return result;
}

itemid_t Logic::addMenuItem(const Item& item) {
    itemid_t id = item.getId();
    if (id <= 0) {
        id = nextItemId_;
    }

    if (findMenuItem(id) != nullptr) {
        throw std::invalid_argument("Menu item with this id already exists");
    }

    Item stored = item.getId() > 0
                      ? item
                      : Item(id,
                             item.getName(),
                             item.getDescription(),
                             item.getPrice(),
                             item.getPortion(),
                             item.getImagePath(),
                             item.getCategory(),
                             item.isAvailable());

    menu_.push_back(stored);
    nextItemId_ = std::max(nextItemId_, id + 1);
    return id;
}

bool Logic::updateMenuItem(const Item& item) {
    Item* existing = findMenuItem(item.getId());
    if (existing == nullptr) {
        return false;
    }
    *existing = item;
    return true;
}

bool Logic::removeMenuItem(itemid_t itemId) {
    auto it = std::find_if(menu_.begin(), menu_.end(), [itemId](const Item& item) {
        return item.getId() == itemId;
    });
    if (it == menu_.end()) {
        return false;
    }

    menu_.erase(it);
    for (auto& entry : users_) {
        entry.second.getCart().setItemQuantity(itemId, 0);
    }
    return true;
}

bool Logic::setItemAvailability(itemid_t itemId, bool available) {
    Item* item = findMenuItem(itemId);
    if (item == nullptr) {
        return false;
    }
    item->setAvailable(available);
    return true;
}

void Logic::clearMenu() {
    menu_.clear();
    nextItemId_ = 1;
    for (auto& entry : users_) {
        entry.second.getCart().clear();
    }
}

User Logic::createUser(const std::string& name,
                       const std::string& phone,
                       const std::string& address) {
    const userid_t id = nextUserId_++;
    User user(id, name, phone, address);
    users_.emplace(id, user);
    return user;
}

bool Logic::hasUser(userid_t userId) const {
    return users_.find(userId) != users_.end();
}

User Logic::viewUser(userid_t userId) const {
    const User* user = findUser(userId);
    if (user == nullptr) {
        throw std::out_of_range("User not found");
    }
    return *user;
}

bool Logic::updateUser(const User& user) {
    User* existing = findUser(user.getId());
    if (existing == nullptr) {
        return false;
    }

    existing->setName(user.getName());
    existing->setPhone(user.getPhone());
    existing->setAddress(user.getAddress());
    return true;
}

int Logic::getUserPoints(userid_t userId) const {
    const User* user = findUser(userId);
    if (user == nullptr) {
        throw std::out_of_range("User not found");
    }
    return user->getPoints();
}

bool Logic::addUserPoints(userid_t userId, int points) {
    User* user = findUser(userId);
    if (user == nullptr || points < 0) {
        return false;
    }
    user->addPoints(points);
    return true;
}

Cart Logic::viewCart(userid_t userId) const {
    const User* user = findUser(userId);
    if (user == nullptr) {
        throw std::out_of_range("User not found");
    }
    return user->getCart();
}

void Logic::addToUserCart(userid_t userId, Item item) {
    User* user = findUser(userId);
    if (user == nullptr) {
        throw std::out_of_range("User not found");
    }
    if (item.getId() <= 0) {
        throw std::invalid_argument("Cart item must have a valid id");
    }

    const Item* menuItem = findMenuItem(item.getId());
    if (menuItem == nullptr) {
        throw std::invalid_argument("Item is not in the menu");
    }
    if (!menuItem->isAvailable()) {
        throw std::invalid_argument("Item is not available");
    }

    user->getCart().addItem(*menuItem);
}

bool Logic::addToUserCart(userid_t userId, itemid_t itemId, int quantity) {
    User* user = findUser(userId);
    if (user == nullptr) {
        return false;
    }

    const Item* item = findMenuItem(itemId);
    if (item == nullptr || !item->isAvailable()) {
        return false;
    }

    user->getCart().addItem(*item, quantity);
    return true;
}

bool Logic::removeFromUserCart(userid_t userId, itemid_t itemId, int quantity) {
    User* user = findUser(userId);
    if (user == nullptr) {
        return false;
    }
    return user->getCart().removeItem(itemId, quantity);
}

bool Logic::setCartItemQuantity(userid_t userId, itemid_t itemId, int quantity) {
    User* user = findUser(userId);
    if (user == nullptr || quantity < 0) {
        return false;
    }
    return user->getCart().setItemQuantity(itemId, quantity);
}

bool Logic::clearUserCart(userid_t userId) {
    User* user = findUser(userId);
    if (user == nullptr) {
        return false;
    }
    user->getCart().clear();
    return true;
}

double Logic::calculateDeliveryCost(userid_t userId) const {
    const User* user = findUser(userId);
    if (user == nullptr) {
        throw std::out_of_range("User not found");
    }
    return deliveryCostForTotal(user->getCart().getTotal());
}

std::optional<orderid_t> Logic::placeOrder(userid_t userId,
                                           const DeliveryInfo& delivery,
                                           int pointsToSpend) {
    if (pointsToSpend < 0 || !delivery.isValid()) {
        return std::nullopt;
    }

    User* user = findUser(userId);
    if (user == nullptr || user->getCart().isEmpty()) {
        return std::nullopt;
    }

    std::vector<CartEntry> orderItems;
    double itemsTotal = 0.0;

    for (const CartEntry& entry : user->getCart().getEntries()) {
        const Item* menuItem = findMenuItem(entry.getItem().getId());
        if (menuItem == nullptr || !menuItem->isAvailable()) {
            return std::nullopt;
        }

        CartEntry validated(*menuItem, entry.getQuantity());
        orderItems.push_back(validated);
        itemsTotal += validated.getSubtotal();
    }

    itemsTotal = roundMoney(itemsTotal);
    const double deliveryCost = deliveryCostForTotal(itemsTotal);

    const int maxDiscountByShare =
        static_cast<int>(std::floor(itemsTotal * MAX_POINTS_DISCOUNT_SHARE));
    const int discount = std::min({pointsToSpend, user->getPoints(), maxDiscountByShare});

    const double total = roundMoney(itemsTotal + deliveryCost - discount);
    const int earned = static_cast<int>(std::floor(total / 100.0)) * POINTS_PER_100_RUBLES;

    Order order(nextOrderId_++,
                userId,
                orderItems,
                itemsTotal,
                deliveryCost,
                discount,
                earned,
                delivery,
                nowString());

    if (discount > 0) {
        user->spendPoints(discount);
    }
    user->addPoints(earned);
    user->addOrderToHistory(order.getId());
    user->getCart().clear();
    orders_.push_back(order);

    return order.getId();
}

std::optional<Order> Logic::findOrder(orderid_t orderId) const {
    auto it = std::find_if(orders_.begin(), orders_.end(), [orderId](const Order& order) {
        return order.getId() == orderId;
    });
    if (it == orders_.end()) {
        return std::nullopt;
    }
    return *it;
}

std::vector<Order> Logic::getUserOrders(userid_t userId) const {
    std::vector<Order> result;
    for (const Order& order : orders_) {
        if (order.getUserId() == userId) {
            result.push_back(order);
        }
    }
    return result;
}

std::vector<Order> Logic::getAllOrders() const { return orders_; }

bool Logic::updateOrderStatus(orderid_t orderId, OrderStatus status) {
    Order* order = findOrderPtr(orderId);
    if (order == nullptr) {
        return false;
    }
    order->setStatus(status);
    return true;
}

bool Logic::cancelOrder(orderid_t orderId) {
    Order* order = findOrderPtr(orderId);
    if (order == nullptr || order->getStatus() == OrderStatus::Cancelled ||
        order->getStatus() == OrderStatus::Completed) {
        return false;
    }

    order->setStatus(OrderStatus::Cancelled);

    User* user = findUser(order->getUserId());
    if (user != nullptr) {
        // Remove points earned for the order, then return the points that
        // were spent as a discount.
        const int earnedToRemove = std::min(order->getPointsEarned(), user->getPoints());
        if (earnedToRemove > 0) {
            user->spendPoints(earnedToRemove);
        }
        if (order->getPointsSpent() > 0) {
            user->addPoints(order->getPointsSpent());
        }
    }

    return true;
}

std::optional<reviewid_t> Logic::addReview(userid_t userId,
                                           int rating,
                                           const std::string& text) {
    if (rating < 1 || rating > 5) {
        return std::nullopt;
    }

    const User* user = findUser(userId);
    if (user == nullptr) {
        return std::nullopt;
    }

    Review review(nextReviewId_++,
                  userId,
                  user->getName(),
                  rating,
                  text,
                  nowString());
    reviews_.push_back(review);
    return review.getId();
}

std::vector<Review> Logic::getReviews() const { return reviews_; }

double Logic::getAverageRating() const {
    if (reviews_.empty()) {
        return 0.0;
    }

    double sum = 0.0;
    for (const Review& review : reviews_) {
        sum += review.getRating();
    }
    return roundMoney(sum / static_cast<double>(reviews_.size()));
}

void Logic::setDeliverySettings(double baseCost, double freeFrom) {
    requireNonNegative(baseCost, "Delivery base cost");
    requireNonNegative(freeFrom, "Free delivery threshold");
    deliveryBaseCost_ = baseCost;
    freeDeliveryFrom_ = freeFrom;
}

double Logic::getDeliveryBaseCost() const { return deliveryBaseCost_; }
double Logic::getFreeDeliveryFrom() const { return freeDeliveryFrom_; }

void Logic::seedDemoMenu() {
    if (!menu_.empty()) {
        return;
    }

    addMenuItem(Item(0,
                     "Эспрессо",
                     "Крепкий кофе из свежеобжаренных зёрен",
                     120.0,
                     "40 мл",
                     "images/espresso.png",
                     ItemCategory::Coffee));
    addMenuItem(Item(0,
                     "Капучино",
                     "Эспрессо с молочной пенкой",
                     220.0,
                     "300 мл",
                     "images/cappuccino.png",
                     ItemCategory::Coffee));
    addMenuItem(Item(0,
                     "Латте",
                     "Эспрессо с молоком",
                     250.0,
                     "350 мл",
                     "images/latte.png",
                     ItemCategory::Coffee));
    addMenuItem(Item(0,
                     "Чёрный чай",
                     "Чёрный чай с лимоном",
                     150.0,
                     "400 мл",
                     "images/tea.png",
                     ItemCategory::Tea));
    addMenuItem(Item(0,
                     "Круассан",
                     "Слоёный круассан с маслом",
                     180.0,
                     "80 г",
                     "images/croissant.png",
                     ItemCategory::Breakfast));
    addMenuItem(Item(0,
                     "Сырники",
                     "Сырники со сметаной и вареньем",
                     290.0,
                     "250 г",
                     "images/syrniki.png",
                     ItemCategory::Breakfast));
    addMenuItem(Item(0,
                     "Паста карбонара",
                     "Паста с беконом, сливками и пармезаном",
                     450.0,
                     "320 г",
                     "images/carbonara.png",
                     ItemCategory::MainCourse));
    addMenuItem(Item(0,
                     "Чизкейк",
                     "Нежный чизкейк с ягодным соусом",
                     320.0,
                     "120 г",
                     "images/cheesecake.png",
                     ItemCategory::Dessert));
}

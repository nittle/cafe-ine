#include "logic.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

int failures = 0;

void check(bool condition, const std::string& name) {
    if (condition) {
        std::cout << "[OK]   " << name << '\n';
    } else {
        std::cerr << "[FAIL] " << name << '\n';
        ++failures;
    }
}

bool approx(double left, double right) {
    return std::fabs(left - right) < 0.001;
}

}  // namespace

int main() {
    Logic logic;

    // --- Menu -------------------------------------------------------------
    check(logic.getItems().size() == 8, "demo menu contains 8 items");
    check(logic.getAvailableItems().size() == 8, "all demo items are available");
    check(logic.getItemsByCategory(ItemCategory::Coffee).size() == 3,
          "three coffee items in the menu");

    const auto cappuccino = logic.searchItems("капучино");
    check(cappuccino.size() == 1, "case-insensitive Cyrillic search works");
    check(cappuccino.front().getName() == "Капучино", "search returns the right item");

    const itemid_t espressoId = logic.searchItems("эспрессо").front().getId();
    const itemid_t pastaId = logic.searchItems("карбонара").front().getId();
    const itemid_t cheesecakeId = logic.searchItems("чизкейк").front().getId();

    // --- Users ------------------------------------------------------------
    const User created = logic.createUser("Даниил", "+7 900 000-00-00", "ул. Пушкина, 1");
    const userid_t userId = created.getId();

    check(userId > 0, "user receives an id");
    check(logic.hasUser(userId), "user is stored");
    check(logic.viewUser(userId).getName() == "Даниил", "user profile can be read");

    User profile = logic.viewUser(userId);
    profile.setPhone("+7 900 111-11-11");
    check(logic.updateUser(profile), "user profile can be updated");
    check(logic.viewUser(userId).getPhone() == "+7 900 111-11-11",
          "updated phone is saved");

    bool missingUserThrows = false;
    try {
        logic.viewUser(9999);
    } catch (const std::out_of_range&) {
        missingUserThrows = true;
    }
    check(missingUserThrows, "unknown user id throws std::out_of_range");

    // --- Cart -------------------------------------------------------------
    check(logic.addToUserCart(userId, espressoId, 2), "add two espressos to cart");
    check(logic.addToUserCart(userId, cappuccino.front().getId(), 1),
          "add one cappuccino to cart");

    Cart cart = logic.viewCart(userId);
    check(cart.getItemCount() == 3, "cart counts units");
    check(cart.getLineCount() == 2, "cart counts lines");
    check(approx(cart.getTotal(), 460.0), "cart total is calculated");
    check(approx(logic.calculateDeliveryCost(userId), 150.0),
          "delivery is paid below the free threshold");

    check(logic.setCartItemQuantity(userId, espressoId, 1), "cart quantity can be changed");
    check(approx(logic.viewCart(userId).getTotal(), 340.0), "cart total after quantity change");

    check(logic.removeFromUserCart(userId, cappuccino.front().getId(), 1),
          "item can be removed from cart");
    check(logic.viewCart(userId).getItemCount() == 1, "cart after removal");

    check(logic.clearUserCart(userId), "cart can be cleared");
    check(logic.viewCart(userId).isEmpty(), "cart is empty after clearing");

    // --- Order ------------------------------------------------------------
    check(logic.addToUserCart(userId, espressoId, 2), "add espressos for the order");
    check(logic.addToUserCart(userId, pastaId, 1), "add pasta for the order");
    check(logic.addToUserCart(userId, cheesecakeId, 1), "add cheesecake for the order");
    check(approx(logic.viewCart(userId).getTotal(), 1010.0), "order items total");

    check(logic.addUserPoints(userId, 50), "bonus points can be added");
    check(logic.getUserPoints(userId) == 50, "bonus points balance");

    DeliveryInfo delivery("ул. Пушкина, 1", "+7 900 111-11-11", "Даниил", "Позвонить заранее");
    check(delivery.isValid(), "delivery info is valid");

    const auto orderId = logic.placeOrder(userId, delivery, 20);
    check(orderId.has_value(), "order is placed");

    const auto order = logic.findOrder(orderId.value());
    check(order.has_value(), "placed order can be found");
    check(approx(order->getItemsTotal(), 1010.0), "order items total is saved");
    check(approx(order->getDeliveryCost(), 0.0), "free delivery above threshold");
    check(order->getPointsSpent() == 20, "points discount is applied");
    check(approx(order->getTotal(), 990.0), "order total with discount");
    check(order->getPointsEarned() == 9, "points are earned for the order");
    check(order->getStatus() == OrderStatus::Created, "new order status");
    check(logic.viewCart(userId).isEmpty(), "cart is cleared after placing an order");
    check(logic.getUserPoints(userId) == 39, "points balance after order");
    check(logic.getUserOrders(userId).size() == 1, "order is in user history");

    check(logic.updateOrderStatus(orderId.value(), OrderStatus::Preparing),
          "order status can be updated");
    check(logic.findOrder(orderId.value())->getStatus() == OrderStatus::Preparing,
          "order status is saved");

    check(logic.cancelOrder(orderId.value()), "order can be cancelled");
    check(logic.findOrder(orderId.value())->getStatus() == OrderStatus::Cancelled,
          "cancelled order status");
    check(logic.getUserPoints(userId) == 50, "points are refunded after cancellation");
    check(!logic.cancelOrder(orderId.value()), "order cannot be cancelled twice");

    check(!logic.placeOrder(userId, delivery).has_value(),
          "empty cart cannot be ordered");
    check(!logic.placeOrder(userId, DeliveryInfo()).has_value(),
          "invalid delivery cannot be ordered");

    // --- Reviews ----------------------------------------------------------
    const auto firstReview = logic.addReview(userId, 5, "Очень вкусно!");
    const auto secondReview = logic.addReview(userId, 4, "Хорошее обслуживание");
    check(firstReview.has_value() && secondReview.has_value(), "reviews can be added");
    check(logic.getReviews().size() == 2, "reviews are stored");
    check(approx(logic.getAverageRating(), 4.5), "average rating is calculated");
    check(!logic.addReview(userId, 6, "Некорректная оценка").has_value(),
          "rating outside 1..5 is rejected");
    check(!logic.addReview(9999, 5, "Неизвестный пользователь").has_value(),
          "review from unknown user is rejected");

    // --- Menu administration ---------------------------------------------
    const itemid_t newItemId = logic.addMenuItem(
        Item(0, "Тестовый напиток", "Напиток для проверки", 100.0, "200 мл"));
    check(newItemId > 0, "new menu item receives an id");

    Item changed = logic.findItem(newItemId).value();
    changed.setPrice(130.0);
    check(logic.updateMenuItem(changed), "menu item can be updated");
    check(approx(logic.findItem(newItemId)->getPrice(), 130.0), "menu item price is saved");

    check(logic.setItemAvailability(newItemId, false), "menu item availability can be changed");
    check(!logic.addToUserCart(userId, newItemId, 1),
          "unavailable item cannot be added to cart");

    check(logic.removeMenuItem(newItemId), "menu item can be removed");
    check(!logic.findItem(newItemId).has_value(), "removed item is gone");

    logic.clearMenu();
    check(logic.getItems().empty(), "menu can be cleared");

    std::cout << '\n';
    if (failures == 0) {
        std::cout << "All logic checks passed.\n";
        return 0;
    }

    std::cerr << failures << " logic check(s) failed.\n";
    return 1;
}

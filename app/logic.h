#ifndef LOGIC_H
#define LOGIC_H
#include<vector>
typedef int userid_t;

class Cart {
    
};

class Item {
};

class User {
};

class Logic {
    public:
    // Get what's in the user's cart
    Cart viewCart(userid_t userId);

    // Add to user's cart
    void addToUserCart(userid_t userId, Item item);
    
    // View user accout
    User viewUser(userid_t userId);

    // Get all available items    
    std::vector<Item> getItems();

    
};

#endif

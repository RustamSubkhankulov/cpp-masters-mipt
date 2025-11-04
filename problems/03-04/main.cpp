// Clients (forward decl)

class Tester_v1;
class Tester_v2;

class Entity final {
private:
    void test_v1() { /* actual impl here */ }
    void test_v2() { /* actual impl here */ }

    friend class EntityAttorney_v1;
    friend class EntityAttorney_v2;
};

// Attorneys

class EntityAttorney_v1 final {
private:
    friend class Tester_v1;
    static void call_test_v1(Entity& e) { e.test_v1(); }
};

class EntityAttorney_v2 final {
private:
    friend class Tester_v2;
    static void call_test_v2(Entity& e) { e.test_v2(); }
};

// Clients

class Tester_v1 final {
public:
    static void invoke(Entity& e) { EntityAttorney_v1::call_test_v1(e); }
};

class Tester_v2 final {
public:
    static void invoke(Entity& e) { EntityAttorney_v2::call_test_v2(e); }
};

int main(int argc, char** argv) {
  Entity e;

  Tester_v1::invoke(e);
  Tester_v2::invoke(e);
}

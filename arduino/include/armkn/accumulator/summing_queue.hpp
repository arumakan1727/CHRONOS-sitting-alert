#include <queue>

/**
 * capacity 件まで要素を保持しつつ、各要素 x の総和を効率的に求める。
 * capacity 件を超える要素が追加されると、最も古い要素が削除される。
 *
 * Elem: 追加する要素の型
 * Sum: 要素の総和の型
 */
namespace armkn {

template <
  typename Elem,
  typename Sum = decltype(std::declval<Elem>() + std::declval<Elem>())>
class SummingQueue {
 public:
  SummingQueue(size_t capacity) : capacity(capacity), sum(), queue() {}

  void add(Elem x) {
    if (queue.size() >= capacity) {
      const auto old = queue.front();
      queue.pop();
      sum -= old;
    }

    queue.push(x);
    sum += x;
  }

  inline Elem get_sum() const { return sum; }

 private:
  size_t capacity;
  Sum sum;
  std::queue<Elem> queue;
};

}  // namespace armkn

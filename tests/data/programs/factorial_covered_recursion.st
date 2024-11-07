language core;

extend with
  #references,
  #arithmetic-operators,
  #sequencing,
  #natural-literals;

fn factorial(f : &(fn(Nat) -> Nat)) -> fn(Nat) -> Nat {
  return fn(n : Nat) {
    return
      f := fn(x : Nat) { return if Nat::iszero(x) then 1 else x * (*f)(x - 1)} ;
      (*f)(n)
  }
}

fn main(n : Nat) -> Nat {
  return factorial(new(fn(x : Nat) { return x }))(n)
}

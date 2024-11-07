language core;
extend with #let-bindings, #references;

fn main(n : Nat) -> Nat {
    return
        let x = new(0) in
        let y = new(0) in
        n;
}

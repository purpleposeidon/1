extern crate lib1;

fn main() {
    let mut stdout = lib1::stdout();
    let p = lib1::fmt!(FORMAT:"hi\n");
    p.write(&mut stdout).expect("write failed");
}

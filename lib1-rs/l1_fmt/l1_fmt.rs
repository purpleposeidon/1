/*
extern crate proc_macro;
use proc_macro::*;
/// ```
/// l1_format_args! {
///     stdout;
///     FieldName "message",
///     FieldValue "{}",
///     FieldText "one";
///     1
/// }
/// ```
#[proc_macro]
pub fn l1_format_args(item: TokenStream) -> TokenStream {
    let mut stdout = vec![];
    let mut item = item.into_iter();
    loop {
        match item.next() {
            None => panic!("expected stdout argument"),
            Some(TokenTree::Punct(p)) if p.as_char() == ';' => break,
            Some(t) => stdout.push(t),
        }
    }
    println!("stdout = {:?}", stdout);

    let mut packets = vec![];
    let mut again = true;
    while again {
        let i = match item.next() {
            None => break,
            Some(TokenTree::Ident(i)) => i,
            e => panic!("expected Identifier (for an l1 Packet type), not {:?}", e),
        };
        let l = match item.next() {
            Some(TokenTree::Literal(l)) => l,
            e => panic!("expected format string, not {:?}", e),
        };
        match item.next() {
            Some(TokenTree::Punct(p)) if p.as_char() == ';' => {
                again = false;
            },
            Some(TokenTree::Punct(p)) if p.as_char() == ',' => {},
            e => panic!("expected ; or , not {:?}", e),
        }
        println!("  {}", l);
        let d = format!("__PacketDelim{}", packets.len());
        packets.push((i, l, d));
    }
    println!("packets = {:?}", packets);
    let mut linkage = vec![];
    for (i, l) in &packets {
    }


    let out = String::new();
    for i in item {
        println!("{:?}", i);
    }
    out.parse().unwrap()
}*/

#![warn(unused)]

#[no_mangle]
pub extern fn get_part_name(actionpart: i32) -> String {
    if actionpart == 0 {
	return "action".to_string()
    } else {
	return "test".to_string()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn partnames() {
        assert_eq!(get_part_name(0), "action");
        assert_eq!(get_part_name(1), "test");
    }
}

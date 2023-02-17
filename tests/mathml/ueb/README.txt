These tests were mostly taken from https://iceb.org/guidelines_for_technical_material_2014.pdf. The numbers refer to the sections in the guide and then the examples in that section (numbers start from 1). For example, "bold_2_7_3" refers to the third example in section 2.7 (Emphasis of digits).

The example that start with "bana_" come from http://www.brailleauthority.org/ueb/ueb_math_guidance/final_for_posting_ueb_math_guidance_may_2019_102419.pdf

UEB has mulitple ways to represent the same expression, so there may other correct interpretations. There are 7 examples where the expected output doesn't match what is in the book. Sometimes this is because the guidance between BANA and ICEB don't match.

The 7 cases are:
alg_3_2_5
    // Acceptable: GTM does not use a G1 start indicator: "⠙⠐⠖⠁⠃⠀⠐⠶⠀⠰⠁⠉"
    // However, BANA says use a word indicator if G1 not in first 3 cells (it is after the '='); use passage if >=2 whitespace
    // This seems like a poor choice in this case since there is only one G1 indicator, but that's the BANA guidance so...
    // Corrected to use passage indicator

set_10_6
    // Note: example uses the wrong char "├" in the display -- should be "⊢"
    // Acceptable: GTM does uses a G1 passage indicator: "⠰⠰⠰⠨⠣⠐⠣⠏⠈⠖⠟⠐⠜⠈⠦⠈⠹⠏⠨⠜⠀⠸⠒⠀⠟⠰⠄"
    // However, the BANA G1 standing alone rule ("...before a single letter standing alone") applies, so start in G2 mode.
    // Corrected to remove the passage indicator

example_11_5_1_3
    // Acceptable: GTM uses a G1 start indicator: "⠰⠰⠋⠶⠐⠣⠭⠐⠜"
    // However, BANA says don't use a word indicator if G1 is in first 3 cells (the ':' needs it)
    // Corrected to avoid word indicator

example_11_5_6
    // Acceptable: GTM uses a G1 passage indicator: "⠰⠰⠰⠸⠣⠐⠣⠭⠂⠀⠽⠐⠜⠀⠸⠳⠀⠭⠐⠖⠽⠀⠐⠶⠀⠼⠋⠸⠜⠰⠄"
    // However, the BANA G1 standing alone rule ("...before a single letter standing alone") applies, so start in G2 mode.
    // Corrected to remove the passage indicator

hat_12_1_7
    // Acceptable: GTM uses a G1 start indicator: "⠰⠰⠠⠁⠠⠃⠐⠱⠠⠉"
    // BANA says use a word indicator if G1 not in first 3 cells (modified it to not count cap indicators since that helps with GTM compatibility)
    // Corrected to skip the G1 indicator at the start (it's debatable as to which is better)

chem_16_2_8
    // Acceptable: GTM does not use a G1 start indicator: "⠠⠉⠁⠐⠣⠠⠕⠠⠓⠐⠜⠰⠢⠼⠃"
    // However, BANA says use a word indicator if G1 not in first 3 cells (it is before the subscript near the end); use passage if >=2 whitespace
    // This seems like a debateable choice in this case since there is only one G1 indicator, but that's the BANA guidance so...
    // Corrected to use word indicator

chem_16_2_9
    // Acceptable: GTM does not use a G1 start indicator: "⠠⠉⠥⠠⠎⠠⠕⠰⠢⠼⠙⠐⠲⠼⠑⠠⠓⠢⠼⠃⠠⠕"
    // However, BANA says use a word indicator if G1 not in first 3 cells (it is before the subscript); use passage if >=2 whitespace
    // This seems like a debatable choice in this case since there is only one G1 indicator, but that's the BANA guidance so...
    // Corrected to use word indicator

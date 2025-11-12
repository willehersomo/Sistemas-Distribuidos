
fn limpar_e_analisar_documento(documento: &str, tamanho_esperado: usize) -> Option<Vec<u32>> {
    let digitos: Vec<u32> = documento.chars().filter_map(|c| c.to_digit(10)).collect();
    
    if digitos.len() != tamanho_esperado || digitos.iter().all(|&d| d == digitos[0]) {
        return None;
    }
    Some(digitos)
}

pub fn e_cpf_valido(cpf: &str) -> bool {
    let digitos = match limpar_e_analisar_documento(cpf, 11) {
        Some(d) => d,
        None => return false,
    };

    let soma1: u32 = digitos.iter().take(9).zip((2..=10).rev()).map(|(&d, peso)| d * peso).sum();
    let verificador1 = (soma1 * 10) % 11 % 10;

    if verificador1 != digitos[9] {
        return false;
    }

    let soma2: u32 = digitos.iter().take(10).zip((2..=11).rev()).map(|(&d, peso)| d * peso).sum();
    let verificador2 = (soma2 * 10) % 11 % 10;

    verificador2 == digitos[10]
}

pub fn e_cnpj_valido(cnpj: &str) -> bool {
    let digitos = match limpar_e_analisar_documento(cnpj, 14) {
        Some(d) => d,
        None => return false,
    };
    
    let pesos1: [u32; 12] = [5, 4, 3, 2, 9, 8, 7, 6, 5, 4, 3, 2];
    let soma1: u32 = digitos.iter().take(12).zip(pesos1.iter()).map(|(&d, &p)| d * p).sum();
    let resto1 = soma1 % 11;
    let verificador1 = if resto1 < 2 { 0 } else { 11 - resto1 };

    if verificador1 != digitos[12] {
        return false;
    }

    let pesos2: [u32; 13] = [6, 5, 4, 3, 2, 9, 8, 7, 6, 5, 4, 3, 2];
    let soma2: u32 = digitos.iter().take(13).zip(pesos2.iter()).map(|(&d, &p)| d * p).sum();
    let resto2 = soma2 % 11;
    let verificador2 = if resto2 < 2 { 0 } else { 11 - resto2 };
    
    verificador2 == digitos[13]
}


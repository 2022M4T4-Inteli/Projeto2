export const errors = {
	
	internal_server_error: {
		code: 1500,
		title: "Erro interno do servidor.",
		description: "A ação solicitada não pôde ser executada devido a um mal funcionamento no servidor. Talvez haja alguma incosistência na conexão com o banco de dados ou algum outro recurso externo."
	},
	user_already_registered: {
		code: 1401,
		title: "Usuário já registrado.",
		description: "O endereço de e-mail informado já encontra-se vinculado a uma outra conta."
	},
	user_not_found: {
		code: 1402,
		title: "Usuário não encontrado.",
		description: "O endereço de e-mail informado não encontra-se vinculado a nenhuma conta."
	},
	invalid_password: {
		code: 1403,
		title: "Senha inválida.",
		description: "A senha informada não corresponde à senha da conta vinculada ao endereço de e-mail informado."
	},
	no_token_provided: {
		code: 1404,
		title: "Nenhum token fornecido.",
		description: "Nenhum token de autenticação foi fornecido."
	},
	invalid_token_format: {
		code: 1405,
		title: "Formato de token inválido.",
		description: "O formato do token fornecido é inválido."
	},
	invalid_token_schema: {
		code: 1406,
		title: "Esquema de token inválido.",
		description: "O esquema do token fornecido é inválido."
	},
	invalid_or_expired_token: {
		code: 1407,
		title: "Token inválido ou expirado.",
		description: "O token fornecido é inválido ou expirou."
	},
	unauthorized: {
		code: 1408,
		title: "Não autorizado.",
		description: "A ação solicitada não pôde ser executada devido a falta de permissão."
	},
	insufficient_permission_level: {
		code: 1409,
		title: "Nível de permissão insuficiente.",
		description: "A ação solicitada não pôde ser executada devido a falta de permissão."
	},
};
# Deploy na AWS Amplify - Sistema de Alarme Arduino

Este guia explica como fazer o deploy da aplicação **Ionic/Angular** do sistema de alarme na AWS Amplify.

## Sobre o Projeto

Este é um projeto Ionic Framework com Angular que se comunica com Arduino via Firebase Realtime Database. O Ionic permite:
- ✅ Funcionar como aplicação web (Progressive Web App)
- ✅ Funcionar como aplicação mobile (iOS/Android via Capacitor)
- ✅ Interface responsiva e otimizada para mobile

## Pré-requisitos

- Conta AWS ativa
- Repositório Git (GitHub, GitLab, Bitbucket ou CodeCommit)
- Código fonte commitado no repositório
- Node.js 18+ instalado (para build local)

## Passo 1: Preparar o Repositório

1. Certifique-se que todos os arquivos estão commitados:
```bash
git add .
git commit -m "Preparar para deploy na AWS Amplify"
git push origin main
```

## Passo 2: Configurar AWS Amplify

1. Acesse o [AWS Amplify Console](https://console.aws.amazon.com/amplify/)
2. Clique em **"New app"** > **"Host web app"**
3. Escolha seu provedor Git (GitHub, GitLab, Bitbucket, etc.)
4. Autorize o AWS Amplify a acessar seus repositórios
5. Selecione o repositório do projeto alarme
6. Selecione a branch **main** (ou sua branch principal)

## Passo 3: Configurações de Build

O arquivo `amplify.yml` já está configurado com:
- **Build command**: `npm run build -- --configuration production`
- **Output directory**: `www`
- **Cache**: `node_modules`
- **Headers customizados**: Cache otimizado para assets estáticos

O AWS Amplify detectará automaticamente este arquivo.

## Passo 4: Variáveis de Ambiente (Opcional)

Caso queira separar as configurações do Firebase por ambiente:

1. No console do Amplify, vá em **Environment variables**
2. Adicione as variáveis necessárias:
   - `FIREBASE_API_KEY`
   - `FIREBASE_AUTH_DOMAIN`
   - `FIREBASE_DATABASE_URL`
   - etc.

**Nota**: Atualmente as credenciais do Firebase estão em `src/firebase.ts`. Para maior segurança em produção, considere usar variáveis de ambiente.

## Passo 5: Configurar Redirects para SPA (IMPORTANTE)

Como Ionic/Angular é uma Single Page Application (SPA), você precisa configurar redirects para que as rotas funcionem:

1. No console do Amplify, clique em **"Save and deploy"** primeiro
2. Após o deploy inicial, vá em **"Rewrites and redirects"** no menu lateral
3. Clique em **"Edit"**
4. Adicione a seguinte regra:
   ```
   Source: </^[^.]+$|\.(?!(css|gif|ico|jpg|js|png|txt|svg|woff|woff2|ttf|map|json|webp)$)([^.]+$)/>
   Target: /index.html
   Type: 200 (Rewrite)
   ```
5. Ou use a regra simples:
   ```
   Source: /<*>
   Target: /index.html
   Type: 200 (Rewrite)
   ```
6. Clique em **"Save"**

**Por que isso é necessário?**
- Sem essa regra, quando você acessar rotas como `/home` ou `/settings` diretamente, receberá erro 404
- O redirect garante que todas as rotas sejam tratadas pelo Angular router

## Passo 6: Deploy

1. O AWS Amplify começará o processo de build e deploy
2. Aguarde o processo completar (normalmente 2-5 minutos)
3. Após concluído, você receberá uma URL do tipo: `https://main.xxxxxx.amplifyapp.com`
4. Teste acessar diferentes rotas da aplicação para garantir que os redirects funcionam

## Passo 7: Configurar Domínio Personalizado (Opcional)

1. No console do Amplify, vá em **Domain management**
2. Clique em **"Add domain"**
3. Configure seu domínio personalizado
4. Atualize os registros DNS conforme instruído

## Estrutura de Build

```
├── amplify.yml          # Configuração de build do Amplify
├── package.json         # Dependências do projeto
├── angular.json         # Configuração do Angular
└── src/
    ├── environments/    # Configurações por ambiente
    └── firebase.ts      # Configuração do Firebase
```

## Comandos Úteis

```bash
# Servir localmente em modo desenvolvimento
npm start
# ou
ionic serve

# Build de produção local para testar
npm run build -- --configuration production

# Testar build de produção localmente
npm run build -- --configuration production && npx http-server www

# Verificar versões
ionic info
```

## Recursos do Ionic na Web

Quando implantado como PWA (Progressive Web App), sua aplicação Ionic terá:

✅ **Interface responsiva** - Funciona em desktop, tablet e mobile
✅ **Componentes otimizados** - UI components do Ionic funcionam perfeitamente na web
✅ **Firebase integrado** - Comunicação em tempo real com Arduino
✅ **Possibilidade de instalação** - Usuários podem "adicionar à tela inicial" em dispositivos móveis
✅ **Modo offline** (se configurado) - Service Workers podem cachear a aplicação

## Troubleshooting

### Build falhando

- Verifique se todas as dependências estão no `package.json`
- Confira os logs de build no console do Amplify
- Teste o build localmente: `npm run build`

### Erro de memória durante build

Adicione a variável de ambiente no Amplify:
- Key: `NODE_OPTIONS`
- Value: `--max_old_space_size=4096`

### Aplicação não carrega

- Verifique se o diretório de saída está correto (`www`)
- **IMPORTANTE**: Configure os redirects para SPA (Passo 5)
- Confira se as rotas do Angular estão configuradas corretamente
- Verifique o console do navegador para erros
- Teste localmente com `npm run build -- --configuration production`

### Componentes Ionic não aparecem corretamente

- Verifique se o CSS do Ionic foi incluído no build
- Confirme que `@ionic/angular` está instalado
- Limpe o cache do navegador (Ctrl+Shift+R ou Cmd+Shift+R)

### Erro 404 ao acessar rotas

- **Solução**: Configure os redirects no console do Amplify (veja Passo 5)
- Isso é essencial para SPAs do Ionic/Angular funcionarem

## Deploy Contínuo

Após a configuração inicial, qualquer push para a branch `main` acionará automaticamente um novo deploy.

## Conexão com Arduino

Certifique-se que:
1. O Arduino está configurado para se conectar ao Firebase
2. As credenciais do Firebase no Arduino correspondem às da aplicação web
3. O Firebase Realtime Database tem as regras de segurança configuradas

## Recursos Adicionais

- [Documentação AWS Amplify](https://docs.aws.amazon.com/amplify/)
- [Ionic Framework](https://ionicframework.com/docs)
- [Firebase Realtime Database](https://firebase.google.com/docs/database)

module.exports = {
    productionSourceMap: process.env.NODE_ENV == 'production' ? false : true,
    chainWebpack: config => {
        config
            .plugin('html')
            .tap(args => {
                args[0].title = "CWA frontend";
                return args;
            })
    }
};
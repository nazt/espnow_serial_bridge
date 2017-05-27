var gulp = require('gulp')
require('babel-register')

let $ = require('gulp-load-plugins')()
let path = require('path')

const plugins = {
  babel: $.babel,
  mocha: $.mocha,
  sourcemaps: $.sourcemaps,
  eslint: $.eslint
}

const paths = {
  src: 'src/**/*.js',
  dist: 'dist',
  tests: 'tests/*.js',
  // Must be absolute or relative to source map
  sourceRoot: path.join(__dirname, 'dist')
}

gulp.task('clean', function () {
  return gulp.src(['.tmp', 'dist'], {read: false}).pipe($.clean())
})

gulp.task('build', ['lint'], function () {
  return gulp.src([paths.src])
  .pipe(plugins.sourcemaps.init())
  .pipe(plugins.babel())
  .pipe(plugins.sourcemaps.write('.', {sourceRoot: paths.sourceRoot}))
  .pipe(gulp.dest(paths.dist))
})

gulp.task('test', function () {
  return gulp.src([paths.tests])
  .pipe(plugins.mocha({
    compilers: plugins.babel
  }))
})

gulp.task('tdd', ['test'], function () {
  return gulp.watch([paths.src, paths.tests], ['test'])
})

gulp.task('watch', ['clean', 'lint', 'build'], function () {
  gulp.watch([paths.src], ['lint', 'build'])
})

gulp.task('lint', () => {
  return gulp.src([paths.src, '!node_modules/**'])
  .pipe(plugins.eslint())
  .pipe(plugins.eslint.format())
  .pipe(plugins.eslint.failAfterError())
})

gulp.task('default', ['clean'], function () {
  gulp.start('build')
})

// gulp.task('tdd-single', function () {
//   return gulp.watch('__tests__/*.js')
//   .on('change', function (file) {
//     gulp.src(file.path)
//     .pipe(mocha({
//       compilers: plugins.babel
//     }))
//   });
// });
